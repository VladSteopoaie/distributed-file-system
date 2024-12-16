#ifndef CACHE_API_HPP
#define CACHE_API_HPP

// external libraries
#include <libmemcached/memcached.h>
#include "server_api.hpp"
#include "cache_protocol.hpp"
#include "utils.hpp"

// standard libraries
#include <iostream>
// #include <fstream>
// #include <vector>
#include <fcntl.h>
#include <unistd.h>
// #include <cstring>

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
    const size_t max_buf_size;
    std::vector<uint8_t> buffer; // buffer to store incoming data

    void handle_error(std::string error);
    void handle_request(const CachePacket& request, CachePacket& response);
    
    int set_cache_object(std::string key, std::string value, time_t expiration, uint32_t flags);
    int set_cache_object(std::string key, std::string value);
    std::string get_cache_object(std::string key);
    
    void set_local_object(const CachePacket& request, CachePacket& response);
    void get_local_object(const CachePacket& request, CachePacket& response);
    
    void read_socket_async();
    void write_socket_async();

public:
    CacheConnectionHandler(asio::io_context& context, memcached_st* mem_client);
    ~CacheConnectionHandler();

    tcp::socket& get_socket();
    void start();
};

//////////////////////////////
// ----[ SERVER CLASS ]---- //
//////////////////////////////
class CacheServer : public GenericServer<CacheConnectionHandler> {
private:
    // memcached stuff
    memcached_st* mem_client;
    std::string mem_conf_string;
    uint16_t memcached_port;
    pid_t memcached_pid;

public:
    CacheServer(const CacheServer&) = delete;
    CacheServer& operator= (const CacheServer&) = delete;
    
    CacheServer(int thread_count, std::string mem_conf_string);
    CacheServer(int thread_count, uint16_t memcached_port);
    CacheServer(int thread_count);
    CacheServer();
    ~CacheServer();

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


#endif