#ifndef CACHE_SERVER_HPP
#define CACHE_SERVER_HPP

#include "cache_connection_handler.hpp"
#include "server_api.hpp"

namespace CacheAPI {
    class CacheServer : public GenericServer<CacheConnectionHandler> {
    private:
        // memcached stuff
        memcached_st* mem_client;
        std::string mem_conf_string;
        std::string file_metadata_dir, dir_metadata_dir;
        uint16_t mem_port;
        pid_t memcached_pid;

    public:
        CacheServer(const CacheServer&) = delete;
        CacheServer& operator= (const CacheServer&) = delete;
        
        CacheServer(int thread_count, std::string mem_conf_string, std::string file_metadata_dir, std::string dir_metadata_dir);
        CacheServer(int thread_count, uint16_t mem_port, std::string file_metadata_dir, std::string dir_metadata_dir);
        CacheServer(int thread_count, std::string file_metadata_dir, std::string dir_metadata_dir);
        CacheServer(std::string file_metadata_dir, std::string dir_metadata_dir);
        ~CacheServer();

        void run(uint16_t port);
    };
}

#endif