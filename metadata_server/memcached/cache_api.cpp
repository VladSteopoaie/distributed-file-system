#include "cache_api.h"
// #include <libmemcached/memcached.h>
// #include <spdlog/spdlog.h>  

////////////////////////////////
// ----[ ABSTRACT CLASS ]---- //
////////////////////////////////

CacheInterface::~CacheInterface() 
{
    memcached_free(mem_client);
    close(master_socket);
}

int CacheInterface::cache_init() {
    
    ////// SOCKETS //////
    master_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (master_socket < 0)
    {
        // std::cerr << "init: " << strerror(errno) << std::endl;
        SPDLOG_ERROR("init: {}", strerror(errno));
        return -1;
    }

    master_addr.sin_family = AF_INET;
    master_addr.sin_port = htons(hostname.port);
    
    if (inet_pton(AF_INET, hostname.address.c_str(), &master_addr.sin_addr) <= 0)
    {
        // std::cerr << "init: " << strerror(errno) << std::endl;
        SPDLOG_ERROR("init: {}", strerror(errno));
        return -1;
    }

    ////// MEMCACHED CONNECTION //////
    if (mem_conf_string.length() == 0)
    {
        SPDLOG_ERROR("init: Configuration string is empty!");
        return -1;
    }

    // check for a configuration file
    size_t start_pos = mem_conf_string.find("--FILE=");
    
    if (start_pos != std::string::npos)
    {
        size_t end_pos = start_pos + std::string("--FILE=").length();
        std::string mem_conf_file = mem_conf_string.substr(end_pos);
        if (read_conf_file(mem_conf_file, mem_conf_string) != 0)
            return -1;
    }

    // check validity of conf_string
    char conf_error[500];

    if (libmemcached_check_configuration(mem_conf_string.c_str(), mem_conf_string.length(), conf_error, sizeof(conf_error)) != MEMCACHED_SUCCESS)
    {
        // std::cerr << conf_error << std::endl;
        SPDLOG_ERROR("init: Invalid configuration string! [{}]", conf_error);
        return -1;
    }

    // initializing connectivity
    mem_client = memcached(mem_conf_string.c_str(), mem_conf_string.length());

    if (mem_client == NULL)
    {
        // std::cerr << "inti(): Memcached error when initializing connectivity!" << std::endl;
        SPDLOG_ERROR("init: Memcached error when initializing connectivity!");
        return -1;
    }

    return 0;
}

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

int CacheServerL::cache_init() override {
    nfds = FD_SETSIZE;
    FD_ZERO(&read_fd_set);
    FD_ZERO(&active_fd_set);

    return CacheInterface::cache_init();
}

int CacheServer::cache_set_object(std::string key, std::string value, time_t expiration, uint32_t flags)
{
    memcached_return_t result = memcached_set(mem_client, 
            key.c_str(), key.size(), 
            value.c_str(), value.size(),
            expiration, flags);

    if (result != MEMCACHED_SUCCESS)
    {
        // std::cerr << "cache_set_object: " << memcached_strerror(mem_client, result) << std::endl;
        SPDLOG_ERROR("cache_set_object: {}", memcached_strerror(mem_client, result));
        return -1;
    }

    return 0;
}

int CacheServer::cache_set_object(std::string key, std::string value)
{
    return cache_set_object(key, value, 0, 0);
}

int CacheServer::cache_listen()
{
    // LISTENING PROCESS
    if (bind(master_socket, (struct sockaddr*) &master_addr, sizeof(master_addr)) == -1)
    {
        // std::cerr << "listen: " << strerror(errno) << std::endl;
        SPDLOG_ERROR("listen: {}", strerror(errno));
        return -1;
    }

    if (listen(master_socket, BACKLOG) == -1) // 128 is backlog value (the maximum number of pending connections)
    {
        // std::cerr << "listen: " << strerror(errno) << std::endl;
        SPDLOG_ERROR("listen: {}", strerror(errno));
        return -1;
    }

    SPDLOG_INFO("Server listening on {}:{}...", hostname.address, hostname.port);

    // ACCEPTING CONNECTIONS

    while (true)
    {

    }

    return 0;
}

// TODO: implement concurrency
int CacheServer::cache_accept()
{
    

    return 0;
}

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
        SPDLOG_ERROR("read_conf_file: {}", strerror(errno));
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
