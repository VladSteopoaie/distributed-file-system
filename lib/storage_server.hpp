#ifndef STORAGE_SERVER_HPP
#define STORAGE_SERVER_HPP

#include "storage_connection_handler.hpp"
#include "generic_server_api.hpp"

namespace StorageAPI {
    class StorageServer : public GenericServer<StorageConnectionHandler> {
    private:
        int rank, comm_size;
        int stripe_size; // stripe size for breaking down large files 
    public:
        StorageServer(const StorageServer&) = delete;
        StorageServer& operator= (const StorageServer&) = delete;
        
        StorageServer();
        StorageServer(int thread_count);
        StorageServer(int thread_count, int stripe_size);
        ~StorageServer();

        void run(uint16_t port);
    };
}

#endif