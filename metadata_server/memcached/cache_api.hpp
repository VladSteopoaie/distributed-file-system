#ifndef CACHE_API_HPP
#define CACHE_API_HPP

// compilation command
// g++ --std=c++20 server_api.cpp cache_api.cpp app.cpp -o app -I/usr/include/spdlog -lfmt -lmemcached

// external libraries
// #define ASIO_STANALONE // non-boost version
// #define ASIO_NO_DEPRECATED // no need for deprecated stuff
// #define ASIO_HAS_STD_COROUTINE // c++20 coroutines needed
// #include <asio.hpp>
// #include <spdlog/spdlog.h>

#include <libmemcached/memcached.h>
#include "server_api.hpp"

// standard libraries
#include <iostream>
#include <fstream>
#include <vector>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <cstring>

using asio::ip::tcp;


////////////////////////////////////
// ----[ CONNECTION HANDLER ]---- //
////////////////////////////////////

class CacheConnectionHandler : public std::enable_shared_from_this<CacheConnectionHandler>
{
private:
    asio::io_context& context;
    tcp::socket socket;
    memcached_st* mem_client;

    int cache_set_object(std::string key, std::string value, time_t expiration, uint32_t flags);
    int cache_set_object(std::string key, std::string value);

public:
    CacheConnectionHandler(asio::io_context& context, memcached_st* mem_client);
    ~CacheConnectionHandler();

    std::string cache_get_object(std::string key);
    tcp::socket& get_socket();
    void start();
};

////////////////////////////////
// ----[ ABSTRACT CLASS ]---- //
////////////////////////////////
// class CacheInterface {
// protected:    
//     // memcached stuff
//     memcached_st* mem_client;
//     std::string mem_conf_string;

//     // asio stuff
//     asio::io_context& context;
//     // tcp::resolver resolver;
//     tcp::acceptor acceptor;
//     asio::signal_set signals;
//     tcp::endpoint endpoint;

// public:
//     CacheInterface(asio::io_context& context, std::string address, int port);
//     CacheInterface(asio::io_context& context, std::string address, int port, std::string mem_conf_string);
//     ~CacheInterface();

//     // virtual int cache_init(); // creates the memcached connection
//     std::string cache_get_object(std::string key);
// };

//////////////////////////////
// ----[ SERVER CLASS ]---- //
//////////////////////////////

// class CacheServer : public CacheInterface {
// private:
//     void do_accept();
// public:
//     CacheServer(const CacheServer&) = delete;
//     CacheServer& operator= (const CacheServer&) = delete;

//     CacheServer(asio::io_context& context, std::string address, int port) : CacheInterface(context, address, port) {};
//     CacheServer(asio::io_context& context, std::string address, int port, std::string mem_conf_string) : CacheInterface(context, address, port, mem_conf_string) {};
    
//     ~CacheServer() {};
//     int cache_set_object(std::string key, std::string value, time_t expiration, uint32_t flags);
//     int cache_set_object(std::string key, std::string value);

//     void run();
// };

class CacheServer : public GenericServer<CacheConnectionHandler> {
private:
    // memcached stuff
    memcached_st* mem_client;
    std::string mem_conf_string;
    uint16_t memcached_port;

public:
    CacheServer(const CacheServer&) = delete;
    CacheServer& operator= (const CacheServer&) = delete;
    
    CacheServer(int thread_count, std::string mem_conf_string);
    CacheServer(int thread_count, uint16_t memcached_port);
    CacheServer();

    void run(uint16_t port);
};

//////////////////////////////
// ----[ CLIENT CLASS ]---- //
//////////////////////////////

// class CacheClient : public CacheInterface {
// private:

// public:
//     CacheClient(hostname_st server_name) : CacheInterface(server_name) {};
//     CacheClient(hostname_st server_name, std::string mem_conf_string) : CacheInterface(server_name, mem_conf_string) {};
//     ~CacheClient() {};
//     int connect_to_server();

// };

void read_conf_file(std::string conf_file, std::string& conf_string);

#endif