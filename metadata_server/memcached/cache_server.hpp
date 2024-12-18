#ifndef CACHE_SERVER_HPP
#define CACHE_SERVER_HPP

#include "cache_connection_handler.hpp"
#include "server_api.hpp"
#include <fcntl.h>
#include <unistd.h>

namespace CacheAPI {
    class CacheServer : public GenericServer<CacheConnectionHandler> {
    private:
        // memcached stuff
        memcached_st* mem_client;
        std::string mem_conf_string;
        uint16_t mem_port;
        pid_t memcached_pid;

    public:
        CacheServer(const CacheServer&) = delete;
        CacheServer& operator= (const CacheServer&) = delete;
        
        CacheServer(int thread_count, std::string mem_conf_string);
        CacheServer(int thread_count, uint16_t mem_port);
        CacheServer(int thread_count);
        CacheServer();
        ~CacheServer();

        void run(uint16_t port);
    };
}

#endif