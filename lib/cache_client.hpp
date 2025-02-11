#ifndef CACHE_CLIENT_HPP
#define CACHE_CLIENT_HPP

#define ASIO_STANALONE // non-boost version
#define ASIO_NO_DEPRECATED // no need for deprecated stuff
#define ASIO_HAS_STD_COROUTINE // c++20 coroutines needed
#include <asio.hpp>
#include "cache_protocol.hpp"

using asio::ip::tcp;

namespace CacheAPI {
    class CacheClient {
    private:
        asio::io_context context;
        tcp::socket socket;
        tcp::resolver resolver;
        std::string address;
        std::string port;

        memcached_st* mem_client;
        uint16_t mem_port;
        std::string mem_conf_string;

        asio::awaitable<void> send_request_async(const CachePacket& request);
        asio::awaitable<void> receive_response_async(CachePacket& response);

    public:
        CacheClient(const CacheClient&) = delete;
        CacheClient& operator= (const CacheClient&) = delete;

        CacheClient();
        CacheClient(std::string mem_conf_file);
        ~CacheClient();

        asio::awaitable<void> connect_async(std::string address, std::string port);
        asio::awaitable<int> set_async(std::string key, std::string value, uint32_t time, uint8_t flags);
        // asio::awaitable<void> set_async(std::string key, std::string value);
        asio::awaitable<std::string> get_async(std::string key);
        std::string get_memcached_object(std::string key); 

        void connect(std::string address, std::string port);
        int set(std::string key, std::string value, uint32_t time, uint8_t flags);
        int set(std::string key, std::string value);
        std::string get(std::string key);
    };
}

#endif