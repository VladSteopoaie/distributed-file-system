#ifndef STORAGE_CLIENT_HPP
#define STORAGE_CLIENT_HPP

#include "generic_client_api.hpp"

using asio::ip::tcp;

namespace StorageAPI {
    class StorageClient : public GenericClient<StoragePacket>{
    private:
        asio::awaitable<int> read_async(const std::string& path, std::vector<uint8_t>& buffer, size_t size, off_t offset);
        asio::awaitable<int> write_async(const std::string& path, const std::vector<uint8_t>& buffer, size_t size, off_t offset);

    public:
        StorageClient(const StorageClient&) = delete;
        StorageClient& operator= (const StorageClient&) = delete;

        StorageClient() = default;
        ~StorageClient() override = default;

        int read(const std::string& path, std::vector<uint8_t>& buffer, size_t size, off_t offset);
        int write(const std::string& path, const std::vector<uint8_t>& buffer, size_t size, off_t offset);
    };
}

#endif