#ifndef CACHE_CONNECTION_HANDLER_HPP
#define CACHE_CONNECTION_HANDLER_HPP

#define ASIO_STANALONE // non-boost version
#define ASIO_NO_DEPRECATED // no need for deprecated stuff
#define ASIO_HAS_STD_COROUTINE // c++20 coroutines needed
#include <asio.hpp>
#include <dirent.h>
#include "cache_protocol.hpp"


using asio::ip::tcp;

namespace CacheAPI {
    class CacheConnectionHandler : public std::enable_shared_from_this<CacheConnectionHandler>
    {
    private:
        asio::io_context& context;
        tcp::socket socket;
        memcached_st* mem_client;
        uint16_t mem_port;
        // const size_t max_buf_size;
        std::vector<uint8_t> buffer; // buffer to store incoming data
        std::string storage_dir;

        void handle_error(std::string error);
        void handle_request(const CachePacket& request, CachePacket& response);

        // memcached related 
        void set_memcached_object(std::string key, std::string value, time_t expiration, uint32_t flags);
        void set_memcached_object(std::string key, std::string value);
        asio::awaitable<void> set_memcached_object_async(std::string key, std::string value, time_t expiration, uint32_t flags);
        std::string get_memcached_object(std::string key);
       
        void read_socket_async();
        void write_socket_async();
        void init_connection(CachePacket& response);

        std::string set_local_file(std::string path, mode_t mode);
        std::string set_local_dir(std::string path, mode_t mode);
        void set(const CachePacket& request, CachePacket& response, bool is_file);
        
        std::string get_local_file(std::string path);
        std::string get_local_dir(std::string path);
        void get(const CachePacket& request, CachePacket& response, bool is_file);

    public:
        CacheConnectionHandler(asio::io_context& context, memcached_st* mem_client, uint16_t mem_port, std::string storage_dir);
        ~CacheConnectionHandler();

        tcp::socket& get_socket();
        void start();
    };
}

#endif