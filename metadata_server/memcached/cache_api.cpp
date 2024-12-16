#include "cache_api.hpp"

template class GenericServer<CacheConnectionHandler>;

////////////////////////////////////
// ----[ CONNECTION HANDLER ]---- //
////////////////////////////////////

// private
void CacheConnectionHandler::handle_error(std::string error)
{
    SPDLOG_ERROR(error);
}

int CacheConnectionHandler::set_cache_object(std::string key, std::string value, time_t expiration, uint32_t flags)
{
    memcached_return_t result = memcached_set(mem_client, 
            key.c_str(), key.size(), 
            value.c_str(), value.size(),
            expiration, flags);

    if (result != MEMCACHED_SUCCESS)
    {
        throw std::runtime_error(std::format("set_cache_object: {}", memcached_strerror(mem_client, result)));
    }

    return 0;
}

int CacheConnectionHandler::set_cache_object(std::string key, std::string value)
{
    return set_cache_object(key, value, 0, 0);
}

std::string CacheConnectionHandler::get_cache_object(std::string key)
{
    memcached_return_t error;
    char* result = memcached_get(mem_client, key.c_str(), key.length(), NULL, NULL, &error);

    if (result == NULL)
    {
        throw std::runtime_error(std::format("get_cache_object: {}", memcached_strerror(mem_client, error)));
    }

    return result;
}

// public
CacheConnectionHandler::CacheConnectionHandler(asio::io_context& context, memcached_st* mem_client)
    : context(context)
    , socket(context)
    , mem_client(mem_client)
    , max_buf_size(8192)
{}

CacheConnectionHandler::~CacheConnectionHandler() {
    socket.close();
    std::cout << "Connection ended!" << std::endl;
}

tcp::socket& CacheConnectionHandler::get_socket() { return socket; }


void CacheConnectionHandler::start() 
{
    tcp::endpoint remote_endpoint =  socket.remote_endpoint();
    SPDLOG_INFO("New connection from {}:{}.",
        remote_endpoint.address().to_string(), remote_endpoint.port());
    read_socket_async();
}

void CacheConnectionHandler::read_socket_async()
{
    auto self(shared_from_this()); // used to keep the connection alive

    // asio::async_read(socket, asio::dynamic_buffer(buffer, max_buf_size),
    buffer.resize(max_buf_size);
    socket.async_read_some(asio::buffer(buffer),
        [this, self] (std::error_code error, size_t bytes_transferred)
        {
            try {
                if (error)
                {
                    throw std::runtime_error(error.message());
                }

                CachePacket request(buffer.data(), bytes_transferred);
                
                CachePacket response;
                handle_request(request, response);

                response.to_buffer(buffer);
                self->write_socket_async();
            }
            catch (std::exception& e)
            {
                self->handle_error(std::format("read_socket_async: {}", e.what()));
            }

        });
}

void CacheConnectionHandler::write_socket_async()
{
    auto self(shared_from_this()); // used to keep the connection alive

    asio::async_write(socket, asio::dynamic_buffer(buffer, max_buf_size),
        [this, self] (std::error_code error, size_t bytes_transferred)
        {
            if (error)
            {
                self->handle_error(std::format("write_socket_async: {}", error.message()));
                return;
            }

            SPDLOG_INFO("Connection handled successfully!");
            // try {
            //     if (error)
            //     {
            //         throw std::runtime_error(error.message());
            //     }
            // }
            // catch (std::exception& e)
            // {
            //     self->handle_error(std::format("write_socket_async: {}", e.what()));
            // }
        });
}

void CacheConnectionHandler::handle_request(const CachePacket& request, CachePacket& response)
{
    if (request.id == 0)
    {
        response.rescode = ResultCode::to_byte(ResultCode::Type::INVPKT);
        return;
    }

    response.id = request.id; 
    response.opcode = request.opcode;

    switch (OperationCode::from_byte(request.opcode))
    {
        case OperationCode::Type::NOP:
            response.rescode = ResultCode::to_byte(ResultCode::Type::SUCCESS);
            break;
        case OperationCode::Type::GET:
            get_local_object(request, response);
            break;
        case OperationCode::Type::SET:
            set_local_object(request, response);
            break;
        case OperationCode::Type::INIT:
            init_connection(response);
            break;
        default:
            response.rescode = ResultCode::to_byte(ResultCode::Type::INVOP);
            break;
    }

}

void CacheConnectionHandler::set_local_object(const CachePacket& request, CachePacket& response)
{
    SPDLOG_INFO("In set_local_object!");
    std::cout << request.to_string() << std::endl;
}

void CacheConnectionHandler::get_local_object(const CachePacket& request, CachePacket& response)
{
    SPDLOG_INFO("In get_local_object!");
    std::cout << request.to_string() << std::endl;
}

void CacheConnectionHandler::init_connection(CachePacket& response)
{
    if (memcached_port <= 0)
    {
        response.rescode = ResultCode::to_byte(ResultCode::Type::NOLOCAL);
        return;
    }

    response.ResultCode::Type::SUCCESS;
    response.message_len = 2; // sending the port which has 2 bytes
    response.message.clear();
    response.message.push_back(memcached_port >> 8);
    response.message.push_back(memcached_port & 0xFF);
}

//////////////////////////////
// ----[ SERVER CLASS ]---- //
//////////////////////////////

CacheServer::CacheServer(int thread_count, std::string mem_conf_string)
    : GenericServer<CacheConnectionHandler>::GenericServer(thread_count)
    , mem_conf_string(mem_conf_string)
    , memcached_pid(-1)
{
    try {
        ////// MEMCACHED CONNECTION //////
        if (mem_conf_string.length() == 0)
        {
            throw std::runtime_error("CacheServer: Configuration string is empty!");
        }

        // check for a configuration file
        size_t start_pos = mem_conf_string.find("--FILE=");
        
        if (start_pos != std::string::npos)
        {
            size_t end_pos = start_pos + std::string("--FILE=").length();
            std::string mem_conf_file = mem_conf_string.substr(end_pos);
            Utils::read_conf_file(mem_conf_file, mem_conf_string);
        }

        // check validity of conf_string
        char conf_error[500];

        if (libmemcached_check_configuration(mem_conf_string.c_str(), mem_conf_string.length(), conf_error, sizeof(conf_error)) != MEMCACHED_SUCCESS)
        {   
            throw std::runtime_error(std::format("CacheServer: Invalid configuration string! [{}]", conf_error));
        }

        // initializing connectivity
        mem_client = memcached(mem_conf_string.c_str(), mem_conf_string.length());

        if (mem_client == NULL)
        {
            throw std::runtime_error("CacheServer: Memcached error when initializing connectivity!");
        }
    } // try
    catch (std::exception& e)
    {
        throw std::runtime_error(e.what());
    }
}

CacheServer::CacheServer(int thread_count, uint16_t memcached_port) 
    : GenericServer<CacheConnectionHandler>::GenericServer(thread_count)
    , memcached_port(memcached_port)
    , mem_conf_string(std::format("--SERVER=127.0.0.1:{}", memcached_port))
{
    // starting a memcached server
    SPDLOG_INFO("CacheServer: Starting memcached server on 0.0.0.0:{}.", memcached_port);
    memcached_pid = fork();

    if (memcached_pid == 0) // child process
    {
        // preparing the command
        char* const args[] = {(char*)"memcached", (char*)"-p", (char*)std::to_string(memcached_port).c_str(), nullptr};
        int result = execvp("memcached", args);

        if (result != 0)
            SPDLOG_ERROR("execvp: {}", strerror(errno));

        exit(0);
    }
    else if (memcached_pid > 0) // parent process
    {
        // check validity of conf_string
        char conf_error[500];

        if (libmemcached_check_configuration(mem_conf_string.c_str(), mem_conf_string.length(), conf_error, sizeof(conf_error)) != MEMCACHED_SUCCESS)
        {
            throw std::runtime_error(std::format("CacheServer: Invalid configuration string! [{}]", conf_error));
        }

        // initializing connectivity
        mem_client = memcached(mem_conf_string.c_str(), mem_conf_string.length());

        if (mem_client == NULL)
        {
            throw std::runtime_error("CacheServer: Memcached error when initializing connectivity!");
        }
    }
    else 
    {
        throw std::runtime_error(std::format("fork: {}", strerror(errno)));
    }
}

CacheServer::CacheServer(int thread_count)
    : CacheServer(thread_count, 11211) // default memcached port -> 11211
{}

CacheServer::CacheServer()
    : CacheServer(1) // default mode -> 1 thread
{}

CacheServer::~CacheServer()
{
    if (memcached_pid > 0) {
        SPDLOG_INFO("Terminating memcached server.");
        kill(memcached_pid, SIGKILL);
    }

    memcached_free(mem_client);
    SPDLOG_INFO("Exiting Cache Server.");
}

void CacheServer::run(uint16_t port) {
    GenericServer<CacheConnectionHandler>::run(port, mem_client);
}


//////////////////////////////
// ----[ CLIENT CLASS ]---- //
//////////////////////////////

CacheClient::CacheClient()
    : CacheClient("")
{}

CacheClient::CacheClient(std::string mem_conf_string)
    : socket(context)
    , resolver(context)
    , mem_conf_string(mem_conf_string)
{}

asio::awaitable<void> CacheClient::connect_async(std::string address, std::string port)
{
    try {
        tcp::resolver::results_type endpoints = 
            co_await resolver.async_resolve(address, port, asio::use_awaitable);
        
        co_await asio::async_connect(socket, endpoints, asio::use_awaitable);
        
        if (mem_conf_string.length() == 0)
        {
            CachePacket request, response;
            request.id = Utils::generate_id();
            request.opcode = OperationCode::to_byte(OperationCode::Type::INIT);

            co_await send_request_async(request);
            co_await receive_response_async(response);

            if (request.id != response.id)
            {
                throw std::runtime_error("Invalid packet id.");
            }

            switch (ResultCode::from_byte(response.rescode))
            {
                case ResultCode::Type::SUCCESS:
                    uint16_t memcached_port;
                    
                    // here I should receive the port number of the memcached server
                    // which is 2 bytes long
                    if (response.message_len != 2)
                    {
                        throw std::runtime_error(std::fromat("Invalid message_len, should be 2, received {}.", response.message_len));
                    }

                    memcached_port = (response.message[0] << 4) + response.message[1];
                    mem_conf_string = std::format("--SERVER={}:{}", address, std::to_string(port));
                    break;
                
                case ResultCode::Type::NOLOCAL:
                    throw std::runtime_error("No local memcached server found, a configuration string is required.");
            }
        }

        // check for a configuration file
        // TODO: create a function for this code, already used twice
        size_t start_pos = mem_conf_string.find("--FILE=");
        
        if (start_pos != std::string::npos)
        {
            size_t end_pos = start_pos + std::string("--FILE=").length();
            std::string mem_conf_file = mem_conf_string.substr(end_pos);
            Utils::read_conf_file(mem_conf_file, mem_conf_string);
        }

        // check validity of conf_string
        char conf_error[500];

        if (libmemcached_check_configuration(mem_conf_string.c_str(), mem_conf_string.length(), conf_error, sizeof(conf_error)) != MEMCACHED_SUCCESS)
        {   
            throw std::runtime_error(std::format("CacheServer: Invalid configuration string! [{}]", conf_error));
        }

        mem_client = memcached(mem_conf_string.c_str(), mem_conf_string.length());

        if (mem_client == NULL)
        {
            throw std::runtime_error("Memcached error when initializing connectivity!");
        }
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(std::fromat("connect_async: {}", e.what()));
    }

    co_return;
}

///////////////////////
// ----[ UTILS ]---- //
///////////////////////

