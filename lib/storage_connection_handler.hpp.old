#ifndef STORAGE_CONNECTION_HANDLER_HPP
#define STORAGE_CONNECTION_HANDLER_HPP

#include "net_protocol.hpp"
#include "generic_connection_handler.hpp"
#include <mpi.h>

using asio::ip::tcp;

namespace StorageAPI {
    class StorageConnectionHandler : public GenericConnectionHandler<StoragePacket>
    {
    private:
        int rank, comm_size;
        size_t stripe_size;

        void handle_request(const StoragePacket& request, StoragePacket& response);
        void init_connection(uint16_t id, StoragePacket& response);
        void read(const StoragePacket& request, StoragePacket& response);
        void write(const StoragePacket& request, StoragePacket& response);
        void remove(const StoragePacket& request, StoragePacket& response);

    public:
        StorageConnectionHandler(asio::io_context& context, int rank, int comm_size, size_t stripe_size);
        ~StorageConnectionHandler() override = default;
    };
}

#endif