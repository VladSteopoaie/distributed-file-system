#ifndef CACHE_API_H
#define CACHE_API_H 

#include <libmemcached/memcached.h>
#include <spdlog/spdlog.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>  
#include <arpa/inet.h>   
#include <cstring>

#define CONF_FILE "./memcached.conf"
#define CONF_MAX_SIZE 1024

#define LOCALHOST "127.0.0.1"
#define DEFAULT_PORT 7331
#define MEMCACHED_DEFAULT_PORT 11211
#define BACKLOG 128

struct hostname_st {
    std::string address;
    in_port_t port;
};

////////////////////////////////
// ----[ ABSTRACT CLASS ]---- //
////////////////////////////////
class CacheInterface {
protected:
    const hostname_st hostname;
    memcached_st* mem_client;
    std::string mem_conf_string;
    int master_socket; // will be a TCP socket
    sockaddr_in master_addr;

public:
    CacheInterface(hostname_st hostname) : hostname(hostname), mem_client(NULL), mem_conf_string("") {};
    CacheInterface(hostname_st hostname, std::string mem_conf_string) : hostname(hostname), mem_client(NULL), mem_conf_string(mem_conf_string) {};
    ~CacheInterface();

    virtual int cache_init(); // creates the memcached connection
    std::string cache_get_object(std::string key);
};

//////////////////////////////
// ----[ SERVER CLASS ]---- //
//////////////////////////////

class CacheServer : public CacheInterface {
private:
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_socket;
    int nfds;
    fd_set read_fd_set, active_fd_set;


public:
    int cache_set_object(std::string key, std::string value, time_t expiration, uint32_t flags);
    int cache_set_object(std::string key, std::string value);
    CacheServer(hostname_st server_name) : CacheInterface(server_name) {};
    CacheServer(hostname_st server_name, std::string mem_conf_string) : CacheInterface(server_name, mem_conf_string) {};
    ~CacheServer() {};

    int cache_init();
    int cache_listen();
    int cache_accept();
};

//////////////////////////////
// ----[ CLIENT CLASS ]---- //
//////////////////////////////

class CacheClient : CacheInterface {
private:

public:
    CacheClient(hostname_st server_name) : CacheInterface(server_name) {};
    CacheClient(hostname_st server_name, std::string mem_conf_string) : CacheInterface(server_name, mem_conf_string) {};
    ~CacheClient() {};
    int connect_to_server();

};

int read_conf_file(std::string conf_file, std::string& conf_string);

#endif