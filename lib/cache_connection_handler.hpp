#ifndef CACHE_CONNECTION_HANDLER_HPP
#define CACHE_CONNECTION_HANDLER_HPP

#define ASIO_STANALONE // non-boost version
#define ASIO_NO_DEPRECATED // no need for deprecated stuff
#define ASIO_HAS_STD_COROUTINE // c++20 coroutines needed
#include <asio.hpp>
#include "cache_protocol.hpp"
#include "file_mngr.hpp"


using asio::ip::tcp;

namespace CacheAPI {
    class CacheConnectionHandler : public std::enable_shared_from_this<CacheConnectionHandler>
    {
    private:
        asio::io_context& context;
        tcp::socket socket;
        memcached_st* mem_client;
        uint16_t mem_port;
        std::vector<uint8_t> buffer; // buffer to store incoming data
        // directories paths to store metadata files
        std::string file_metadata_dir;
        std::string dir_metadata_dir;

        void handle_request(const CachePacket& request, CachePacket& response);

        void update_parent_dir(const std::string& path);
        void update_memcached_object(const std::string& key, const std::string& path, time_t expiration, uint32_t flags);
        asio::awaitable<void> update_memcached_object_async(const std::string& key, const std::string& path, time_t expiration, uint32_t flags);

        // memcached related 
        void set_memcached_object(const std::string& key, const std::string& value, time_t expiration, uint32_t flags);
        void set_memcached_object(const std::string& key, const std::string& value);
        asio::awaitable<void> set_memcached_object_async(const std::string& key, const std::string& value, time_t expiration, uint32_t flags);

        std::string get_memcached_object(const std::string& key);

        void remove_memcached_object(const std::string& key);
        asio::awaitable<void> remove_memcached_object_async(const std::string& key);

        void read_socket_async();
        void write_socket_async();
        void init_connection(CachePacket& response);

        void set(const CachePacket& request, CachePacket& response, bool is_file);
        void get(const CachePacket& request, CachePacket& response, bool is_file);
        void remove(const CachePacket& request, CachePacket& response, bool is_file);
        void update(const CachePacket& request, CachePacket& response);

    public:
        CacheConnectionHandler(
            asio::io_context& context, 
            memcached_st* mem_client, 
            uint16_t mem_port, 
            std::string file_metadata_dir,
            std::string dir_metadata_dir
        );
        
        ~CacheConnectionHandler();

        tcp::socket& get_socket();
        void start();
    };
}

#endif