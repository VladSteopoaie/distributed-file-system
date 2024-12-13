#include "cache_api.hpp"

template class GenericServer<CacheConnectionHandler>;

////////////////////////////////////
// ----[ CONNECTION HANDLER ]---- //
////////////////////////////////////

// private
int CacheConnectionHandler::cache_set_object(std::string key, std::string value, time_t expiration, uint32_t flags)
{
    memcached_return_t result = memcached_set(mem_client, 
            key.c_str(), key.size(), 
            value.c_str(), value.size(),
            expiration, flags);

    if (result != MEMCACHED_SUCCESS)
    {
        throw std::runtime_error(std::format("cache_set_object: {}", memcached_strerror(mem_client, result)));
    }

    return 0;
}

int CacheConnectionHandler::cache_set_object(std::string key, std::string value)
{
    return cache_set_object(key, value, 0, 0);
}

std::string CacheConnectionHandler::cache_get_object(std::string key)
{
    memcached_return_t error;
    char* result = memcached_get(mem_client, key.c_str(), key.length(), NULL, NULL, &error);

    if (result == NULL)
    {
        throw std::runtime_error(std::format("cache_get_object: {}", memcached_strerror(mem_client, error)));
    }

    return result;
}

// public
CacheConnectionHandler::CacheConnectionHandler(asio::io_context& context, memcached_st* mem_client)
    : context(context)
    , socket(context)
    , mem_client(mem_client)
{}

CacheConnectionHandler::~CacheConnectionHandler() {
    std::cout << "Connection ended!" << std::endl;
}

tcp::socket& CacheConnectionHandler::get_socket() { return socket; }


void CacheConnectionHandler::start() 
{
    std::cout << "Hello" << std::endl;
    socket.close();
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
            read_conf_file(mem_conf_file, mem_conf_string);
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
        // managing signal behaviour regarding the child process
        // struct sigaction sa;
        // sa.sa_handler = [](int) { kill(memcached_pid, SIGKILL); }
        // sigaction(SIGHUP, &sa, nullptr);

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

///////////////////////
// ----[ UTILS ]---- //
///////////////////////

void read_conf_file(std::string conf_file, std::string& conf_string)
{
    try {
        std::ifstream file (conf_file);

        if (!file)
        {
            throw std::runtime_error(std::format("read_conf_file: {}", strerror(errno)));
        }

        std::string content;
        conf_string.clear();
        
        file >> content;
        conf_string += content;

        while (file >> content)
            conf_string += " " + content;

        file.close();
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(e.what());
    }
}



////////////////////////////////
// ----[ ABSTRACT CLASS ]---- //
////////////////////////////////

// CacheInterface::CacheInterface(asio::io_context& context, std::string address, int port, std::string mem_conf_string) 
//     : context(context)
//     , acceptor(context)
//     , signals(context)
//     // , resolver(context)
//     , mem_client(NULL)
//     , mem_conf_string(mem_conf_string)
// {
//     try{

//         ////// ASIO INITIALIZATION //////
//         tcp::resolver resolver(context);
//         endpoint = *resolver.resolve(address, std::to_string(port)).begin();
//         acceptor.open(endpoint.protocol());
//         acceptor.set_option(tcp::acceptor::reuse_address(true));

//         signals.add(SIGINT);
//         signals.add(SIGTERM);
//         signals.async_wait([&](auto, auto){ 
//             SPDLOG_WARN("Interrupt: Exiting ...");
//             context.stop(); 
//         });

//         ////// MEMCACHED CONNECTION //////
//         if (mem_conf_string.length() == 0)
//         {
//             throw std::runtime_error("init: Configuration string is empty!");
//         }

//         // check for a configuration file
//         size_t start_pos = mem_conf_string.find("--FILE=");
        
//         if (start_pos != std::string::npos)
//         {
//             size_t end_pos = start_pos + std::string("--FILE=").length();
//             std::string mem_conf_file = mem_conf_string.substr(end_pos);
//             if (read_conf_file(mem_conf_file, mem_conf_string) != 0)
//                 throw std::runtime_error(std::format("read_conf_file: {}", strerror(errno)));
//         }

//         // check validity of conf_string
//         char conf_error[500];

//         if (libmemcached_check_configuration(mem_conf_string.c_str(), mem_conf_string.length(), conf_error, sizeof(conf_error)) != MEMCACHED_SUCCESS)
//         {
//             throw std::runtime_error(std::format("init: Invalid configuration string! [{}]", conf_error));
//         }

//         // initializing connectivity
//         mem_client = memcached(mem_conf_string.c_str(), mem_conf_string.length());

//         if (mem_client == NULL)
//         {
//             throw std::runtime_error("CacheServer: Memcached error when initializing connectivity!");
//         }
//     } // try
//     catch (std::exception& e)
//     {
//         throw std::runtime_error(e.what());
//     }
// }

// CacheInterface::CacheInterface(asio::io_context& context, std::string address, int port)
//     : CacheInterface(context, address, port, "")
// {}

// CacheInterface::~CacheInterface() 
// {
//     memcached_free(mem_client);
//     context.stop();
// }

// std::string CacheInterface::cache_get_object(std::string key)
// {
//     memcached_return_t error;
//     char* result = memcached_get(mem_client, key.c_str(), key.length(), NULL, NULL, &error);

//     if (result == NULL)
//     {
//         // SPDLOG_ERROR("cache_get_object: {}", memcached_strerror(mem_client, error));
//         // return std::string();
//         throw std::runtime_error(std::format("cache_get_object: {}", memcached_strerror(mem_client, error)));
//     }

//     return result;
// }

// int CacheServer::cache_set_object(std::string key, std::string value, time_t expiration, uint32_t flags)
// {
//     memcached_return_t result = memcached_set(mem_client, 
//             key.c_str(), key.size(), 
//             value.c_str(), value.size(),
//             expiration, flags);

//     if (result != MEMCACHED_SUCCESS)
//     {
//         // std::cerr << "cache_set_object: " << memcached_strerror(mem_client, result) << std::endl;
//         // SPDLOG_ERROR("cache_set_object: {}", memcached_strerror(mem_client, result));
//         throw std::runtime_error(std::format("cache_set_object: {}", memcached_strerror(mem_client, result)));
//         // return -1;
//     }

//     return 0;
// }

// int CacheServer::cache_set_object(std::string key, std::string value)
// {
//     return cache_set_object(key, value, 0, 0);
// }

// void CacheServer::do_accept()
// {
//     acceptor.async_accept([&] (asio::error_code ec, tcp::socket socket) {
//         if (!acceptor.is_open())
//             return;
        
//         if (ec)
//         {
//             throw std::runtime_error(ec.message());
//         }

//         SPDLOG_INFO("Connection from: {}:{}", socket.remote_endpoint().address().to_string(), socket.remote_endpoint().port());
        
        

//         do_accept();
//     });
// }

// void CacheServer::run()
// {
//     try 
//     {
//         acceptor.bind(endpoint);
//         acceptor.listen();

//         SPDLOG_INFO("Server running on {}:{}", endpoint.address().to_string(), endpoint.port());

//         do_accept();

//         context.run();
//     }
//     catch (std::exception& e)
//     {
//         context.stop();
//         throw std::runtime_error(e.what());
//     }
// }
