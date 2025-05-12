#ifndef STORAGE_CLIENT_HPP
#define STORAGE_CLIENT_HPP

#include "generic_client_api.hpp"

using asio::ip::tcp;

namespace StorageAPI {
    class StorageClient : public GenericClient<StoragePacket>{
    private:
        size_t stripe_size;
        asio::awaitable<int> read_async(const std::string& path, char* buffer, size_t size, off_t offset);
        asio::awaitable<int> write_async(
              const std::string& path
            , const char* buffer
            , size_t size
            , off_t offset);
        
        asio::awaitable<int> remove_async(const std::string& path);
    public:
        StorageClient(const StorageClient&) = delete;
        StorageClient& operator= (const StorageClient&) = delete;

        StorageClient();
        StorageClient(size_t stripe_size);
        StorageClient(int thread_count, size_t stripe_size);
        ~StorageClient() override = default;

        int read(const std::string& path, char* buffer, size_t size, off_t offset);
        // int write(const std::string& path, const std::vector<uint8_t>& buffer, size_t size, off_t offset);
        int write(const std::string& path, const char* buffer, size_t size, off_t offset);
        // int write_stripes(const std::string& path, const std::vector<uint8_t>& buffer, size_t size, off_t offset);
        int remove(const std::string& path);
    };
}

#endif