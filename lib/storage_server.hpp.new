#ifndef STORAGE_SERVER_HPP
#define STORAGE_SERVER_HPP

#include "storage_connection_handler.hpp"
#include "generic_server_api.hpp"

namespace StorageAPI {
    class StorageServer : public GenericServer<StorageConnectionHandler> {
    private:
        std::vector<Utils::ConnectionInfo<StoragePacket>> connections;
        int stripe_size; // stripe size for breaking down large files
        // std::vector<std::unique_ptr<asio::ip::tcp::socket>> sockets;
        void init();
    public:
        StorageServer(const StorageServer&) = delete;
        StorageServer& operator= (const StorageServer&) = delete;
        
        // StorageServer(std::vector<Utils::ConnectionInfo<StoragePacket>>&& connections);
        // StorageServer(int thread_count, std::vector<Utils::ConnectionInfo<StoragePacket>>&& connections);
        StorageServer(int thread_count, int stripe_size, std::vector<Utils::ConnectionInfo<StoragePacket>> connections);
        ~StorageServer();

        void run(uint16_t port);
    };
}

#endif