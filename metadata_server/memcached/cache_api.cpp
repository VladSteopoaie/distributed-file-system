#include "cache_api.hpp"
// #include <libmemcached/memcached.h>
// #include <spdlog/spdlog.h>  

////////////////////////////////
// ----[ ABSTRACT CLASS ]---- //
////////////////////////////////

CacheInterface::CacheInterface(asio::io_context& context, std::string address, int port, std::string mem_conf_string) 
    : context(context)
    , acceptor(context)
    , signals(context)
    , resolver(context)
    , mem_client(NULL)
    , mem_conf_string(mem_conf_string)
{
    try{

        ////// ASIO INITIALIZATION //////
        endpoint = *resolver.resolve(address, std::to_string(port)).begin();
        acceptor.open(endpoint.protocol());
        acceptor.set_option(tcp::acceptor::reuse_address(true));

        signals.add(SIGINT);
        signals.add(SIGTERM);
        signals.async_wait([&](auto, auto){ 
            SPDLOG_WARN("Intrerupt: Exiting ...");
            context.stop(); 
        });

        ////// MEMCACHED CONNECTION //////
        if (mem_conf_string.length() == 0)
        {
            // SPDLOG_ERROR("init: Configuration string is empty!");
            throw std::runtime_error("init: Configuration string is empty!");
            // return -1;
        }

        // check for a configuration file
        size_t start_pos = mem_conf_string.find("--FILE=");
        
        if (start_pos != std::string::npos)
        {
            size_t end_pos = start_pos + std::string("--FILE=").length();
            std::string mem_conf_file = mem_conf_string.substr(end_pos);
            if (read_conf_file(mem_conf_file, mem_conf_string) != 0)
                throw std::runtime_error(std::format("read_conf_file: {}", strerror(errno)));
                // return -1;
        }

        // check validity of conf_string
        char conf_error[500];

        if (libmemcached_check_configuration(mem_conf_string.c_str(), mem_conf_string.length(), conf_error, sizeof(conf_error)) != MEMCACHED_SUCCESS)
        {
            // std::cerr << conf_error << std::endl;
            // SPDLOG_ERROR("init: Invalid configuration string! [{}]", conf_error);
            throw std::runtime_error(std::format("init: Invalid configuration string! [{}]", conf_error));
            // return -1;
        }

        // initializing connectivity
        mem_client = memcached(mem_conf_string.c_str(), mem_conf_string.length());

        if (mem_client == NULL)
        {
            // std::cerr << "inti(): Memcached error when initializing connectivity!" << std::endl;
            // SPDLOG_ERROR("init: Memcached error when initializing connectivity!");
            throw std::runtime_error("Server constructor: Memcached error when initializing connectivity!");
            // return -1;
        }
    } // try
    catch (std::exception& e)
    {
        throw std::runtime_error(e.what());
    }
}

CacheInterface::CacheInterface(asio::io_context& context, std::string address, int port)
    : CacheInterface(context, address, port, "")
{}

// {}


CacheInterface::~CacheInterface() 
{
    memcached_free(mem_client);
}

// int CacheInterface::cache_init() 
// {


    

//     return 0;
// }

std::string CacheInterface::cache_get_object(std::string key)
{
    memcached_return_t error;
    char* result = memcached_get(mem_client, key.c_str(), key.length(), NULL, NULL, &error);

    if (result == NULL)
    {
        // std::cerr << "cache_get_object: " << memcached_strerror(mem_client, error) << std::endl;
        SPDLOG_ERROR("cache_get_object: {}", memcached_strerror(mem_client, error));
        return std::string();
    }

    return result;
}

//////////////////////////////
// ----[ SERVER CLASS ]---- //
//////////////////////////////

// int CacheServerL::cache_init() override {
//     nfds = FD_SETSIZE;
//     FD_ZERO(&read_fd_set);
//     FD_ZERO(&active_fd_set);

//     return CacheInterface::cache_init();
// }

int CacheServer::cache_set_object(std::string key, std::string value, time_t expiration, uint32_t flags)
{
    memcached_return_t result = memcached_set(mem_client, 
            key.c_str(), key.size(), 
            value.c_str(), value.size(),
            expiration, flags);

    if (result != MEMCACHED_SUCCESS)
    {
        // std::cerr << "cache_set_object: " << memcached_strerror(mem_client, result) << std::endl;
        // SPDLOG_ERROR("cache_set_object: {}", memcached_strerror(mem_client, result));
        throw std::runtime_error(std::format("cache_set_object: {}", memcached_strerror(mem_client, result)));
        // return -1;
    }

    return 0;
}

int CacheServer::cache_set_object(std::string key, std::string value)
{
    return cache_set_object(key, value, 0, 0);
}

// int CacheServer::cache_listen()
// {
//     // LISTENING PROCESS
//     if (bind(master_socket, (struct sockaddr*) &master_addr, sizeof(master_addr)) == -1)
//     {
//         // std::cerr << "listen: " << strerror(errno) << std::endl;
//         // SPDLOG_ERROR("listen: {}", strerror(errno));
//         throw std::runtime_error(std::format("listen: {}", strerror(errno)));
//         // return -1;
//     }

//     if (listen(master_socket, BACKLOG) == -1) // 128 is backlog value (the maximum number of pending connections)
//     {
//         // std::cerr << "listen: " << strerror(errno) << std::endl;
//         // SPDLOG_ERROR("listen: {}", strerror(errno));
//         // return -1;
//         throw std::runtime_error(std::format("listen: {}", strerror(errno)));
//     }

//     SPDLOG_INFO("Server listening on {}:{}...", endpoint.address().to_string(), endpoint.port().to_string());

//     // ACCEPTING CONNECTIONS

//     // while (true)
//     // {
//     //     ;
//     // }

//     return 0;
// }

// int CacheServer::cache_accept()
// {
    

//     return 0;
// }

//////////////////////////////
// ----[ CLIENT CLASS ]---- //
//////////////////////////////

///////////////////////
// ----[ UTILS ]---- //
///////////////////////

int read_conf_file(std::string conf_file, std::string& conf_string)
{
    std::ifstream file (conf_file);

    if (!file)
    {
        // std::cerr << "read_conf_file: " << strerror(errno) << std::endl;
        // SPDLOG_ERROR("read_conf_file: {}", strerror(errno));
        return -1;
    }

    std::string content;
    conf_string.clear();
    
    file >> content;
    conf_string += content;

    while (file >> content)
        conf_string += " " + content;

    file.close();
    return 0;
}
