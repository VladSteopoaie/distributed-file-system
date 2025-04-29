#ifndef CACHE_CLIENT_HPP
#define CACHE_CLIENT_HPP

#include "generic_client_api.hpp"
using asio::ip::tcp;

namespace CacheAPI {
    class CacheClient : public GenericClient<CachePacket> {
    private:
        memcached_st* mem_client;
        uint16_t mem_port;
        std::string mem_conf_string;

        std::string get_memcached_object(const std::string& key); 

        asio::awaitable<int> set_async(const std::string& key, const std::string& value, uint32_t time, uint8_t flags, bool is_file);
        int set(const std::string& key, const std::string& value, uint32_t time, uint8_t flags, bool is_file);
        int set(const std::string& key, const std::string& value, bool is_file);
  
        asio::awaitable<std::string> get_async(const std::string& key, bool is_file);
        std::string get(const std::string& key, bool is_file);

        asio::awaitable<int> remove_async(const std::string& key, bool is_file);
        int remove(const std::string& key, bool is_file);

        asio::awaitable<int> update_async(const std::string& key, const UpdateCommand& command);
        int update(const std::string& key, const UpdateCommand& command);
    public:
        CacheClient();
        CacheClient(const std::string& mem_conf_file);
        ~CacheClient() override;

        asio::awaitable<void> connect_async(const std::string& address, const std::string& port) override;
        
        int set_file(const std::string& key, const std::string& value);
        int set_dir(const std::string& key, const std::string& value);
        std::string get_file(const std::string& key);
        std::string get_dir(const std::string& key);
        int remove_file(const std::string& key);
        int remove_dir(const std::string& key);
        int chmod(const std::string& key, mode_t new_mode);
        int chown(const std::string& key, uid_t new_uid, gid_t new_gid);
        int rename(const std::string& old_key, const std::string& new_key);
    };
}

#endif