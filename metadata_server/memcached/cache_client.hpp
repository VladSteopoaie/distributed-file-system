#ifndef CACHE_CLIENT_HPP
#define CACHE_CLIENT_HPP

#define ASIO_STANALONE // non-boost version
#define ASIO_NO_DEPRECATED // no need for deprecated stuff
#define ASIO_HAS_STD_COROUTINE // c++20 coroutines needed
#include <asio.hpp>
#include <spdlog/spdlog.h>

using asio::ip::tcp;
// #include "utils.hpp"
#include "cache_protocol.hpp"
#include <iostream> //temp

namespace CacheAPI {
    class CacheClient {
    private:
        asio::io_context context;
        tcp::socket socket;
        tcp::resolver resolver;

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
        void connect(std::string address, std::string port);
    };
}

#endif