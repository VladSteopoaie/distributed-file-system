#ifndef CACHE_CONNECTION_HANDLER_HPP
#define CACHE_CONNECTION_HANDLER_HPP

#include "net_protocol.hpp"
#include "generic_connection_handler.hpp"
#include "file_mngr.hpp"


using asio::ip::tcp;

namespace CacheAPI {
    class CacheConnectionHandler : public GenericConnectionHandler<CachePacket>
    {
    private:
        memcached_st* mem_client;
        uint16_t mem_port;
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
        
        ~CacheConnectionHandler() override = default;
    };
}

#endif