#include "cache_connection_handler.hpp"

using namespace CacheAPI;

// private
void CacheConnectionHandler::handle_error(std::string error)
{
    SPDLOG_ERROR(error);
}

void CacheConnectionHandler::set_memcached_object(std::string key, std::string value, time_t expiration, uint32_t flags)
{
    memcached_return_t result = memcached_set(mem_client, 
            key.c_str(), key.size(), 
            value.c_str(), value.size(),
            expiration, flags);

    if (result != MEMCACHED_SUCCESS)
    {
        throw std::runtime_error(std::format("set_memcached_object: {}", memcached_strerror(mem_client, result)));
    }
}

void CacheConnectionHandler::set_memcached_object(std::string key, std::string value)
{
    try {
        set_memcached_object(key, value, 0, 0);
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(e.what());
    }
}

asio::awaitable<void> CacheConnectionHandler::set_memcached_object_async(std::string key, std::string value, time_t expiration, uint32_t flags)
{
    SPDLOG_DEBUG("In set_memcached_object_async");
    try {
        set_memcached_object(key, value, expiration, flags);
    }
    catch (std::exception& e)
    {
        SPDLOG_ERROR(std::format("set_memcached_object_async: {}", e.what()));
    }

    co_return;
}

std::string CacheConnectionHandler::get_memcached_object(std::string key)
{
    memcached_return_t error;
    char* result = memcached_get(mem_client, key.c_str(), key.length(), NULL, NULL, &error);

    if (result == NULL)
    {
        throw std::runtime_error(std::format("get_memcached_object: {}", memcached_strerror(mem_client, error)));
    }

    return result;
}

// public
CacheConnectionHandler::CacheConnectionHandler(asio::io_context& context, memcached_st* mem_client, uint16_t mem_port, std::string storage_dir)
    : context(context)
    , socket(context)
    , mem_client(mem_client)
    , mem_port(mem_port)
    , storage_dir(storage_dir)
{}

CacheConnectionHandler::~CacheConnectionHandler() {
    socket.close();
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

    buffer.resize(CachePacket::max_packet_size);
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

    asio::async_write(socket, asio::buffer(buffer),
        [this, self] (std::error_code error, size_t bytes_transferred)
        {
            if (error)
            {
                self->handle_error(std::format("write_socket_async: {}", error.message()));
                return;
            }

            SPDLOG_INFO("Connection handled successfully!");
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

    SPDLOG_DEBUG(std::format("Processing: {}", OperationCode::to_string(OperationCode::from_byte(request.opcode))));
    switch (OperationCode::from_byte(request.opcode))
    {
        case OperationCode::Type::NOP:
            response.rescode = ResultCode::to_byte(ResultCode::Type::SUCCESS);
            break;
        case OperationCode::Type::GET:
            get_object(request, response);
            break;
        case OperationCode::Type::SET:
            set_object(request, response);
            break;
        case OperationCode::Type::INIT:
            init_connection(response);
            break;
        default:
            response.rescode = ResultCode::to_byte(ResultCode::Type::INVOP);
            break;
    }

}

void CacheConnectionHandler::set_object(const CachePacket& request, CachePacket& response)
{
    try {
        uint32_t flags = request.flags;
        time_t time = static_cast<time_t>(request.time);
        std::string path = Utils::get_string_from_byte_array(request.key);
        mode_t mode = std::stoul(Utils::get_string_from_byte_array(request.value));
        // std::cout << mode << std::endl;
        SPDLOG_DEBUG(mode);
        std::string value = set_local_file(path, mode);
        set_memcached_object(path, value, time, flags);

        response.rescode = ResultCode::to_byte(ResultCode::Type::SUCCESS);
    }
    catch (std::exception& e)
    {
        std::string message = std::format("set_object: {}", e.what());
        SPDLOG_ERROR(message);
        response.rescode = ResultCode::to_byte(ResultCode::Type::ERRMSG);
        response.message_len = 4; // static_cast<uint16_t>(message.length());
        response.message = Utils::get_byte_array_from_int(errno);// Utils::get_byte_array_from_string(message);
    }
}

std::string CacheConnectionHandler::set_local_file(std::string path, mode_t mode)
{
    std::string file_path = storage_dir + path;
    std::string result;
    Stat file_proto;
    struct stat file_stat;
    int fd;

    fd = open(file_path.c_str(), O_CREAT | O_RDWR, mode);
    SPDLOG_DEBUG(fd);
    if (fd < 0)
        throw std::runtime_error(std::format("set_local_file: {}", std::strerror(errno)));

    if (fstat(fd, &file_stat) != 0)
        throw std::runtime_error(std::format("set_local_file: {}", std::strerror(errno)));
    // std::cout << 1 << std::endl;
    Utils::struct_stat_to_proto(&file_stat, file_proto);
    file_proto.SerializeToString(&result);
    // std::cout << result << std::endl;
    // std::cout << result.length() << std::endl;
    if (write(fd, result.c_str(), result.length()) < 0)
        throw std::runtime_error(std::format("set_local_file: {}", std::strerror(errno)));

    return result;
}

void CacheConnectionHandler::get_object(const CachePacket& request, CachePacket& response)
{
    try {
        std::string key = Utils::get_string_from_byte_array(request.key);
        std::string value = get_local_file(key);
        
        if (value.length() > 0)
            asio::co_spawn(context, set_memcached_object_async(key, value, 0, 0), asio::detached);

        response.rescode = ResultCode::to_byte(ResultCode::Type::SUCCESS);
        response.value_len = value.length();
        response.value = Utils::get_byte_array_from_string(value);
    }
    catch (std::exception& e)
    {
        std::string message = std::format("get_object: {}", e.what());
        SPDLOG_ERROR(message);
        response.rescode = ResultCode::to_byte(ResultCode::Type::ERRMSG);
        response.message_len = static_cast<uint16_t>(message.length());
        response.message = Utils::get_byte_array_from_string(message);
    }

}

std::string CacheConnectionHandler::get_local_file(std::string key)
{
    std::string file_path = storage_dir + key;
    std::ifstream file(file_path);

    if (!file)
    {
        throw std::runtime_error(std::format("get_local_file: {}", std::strerror(errno)));
    }

    std::ostringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

void CacheConnectionHandler::init_connection(CachePacket& response)
{
    // sending the memcached port if it's set
    if (mem_port <= 0)
    {
        response.rescode = ResultCode::to_byte(ResultCode::Type::NOLOCAL);
        return;
    }

    response.rescode = ResultCode::Type::SUCCESS;
    response.message_len = 2; // sending the port which has 2 bytes
    response.message.clear();
    response.message.push_back(mem_port >> 8);
    response.message.push_back(mem_port & 0xFF);
}
