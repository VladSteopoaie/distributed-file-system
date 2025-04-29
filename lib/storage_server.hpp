#ifndef STORAGE_SERVER_HPP
#define STORAGE_SERVER_HPP

#include "storage_connection_handler.hpp"
#include "generic_server_api.hpp"

namespace StorageAPI {
    class StorageServer : public GenericServer<StorageConnectionHandler> {
    public:
        StorageServer(const StorageServer&) = delete;
        StorageServer& operator= (const StorageServer&) = delete;
        
        StorageServer(int thread_count);
        ~StorageServer();

        void run(uint16_t port);
    };
}

#endif