#ifndef STORAGE_CONNECTION_HANDLER_HPP
#define STORAGE_CONNECTION_HANDLER_HPP

#include "net_protocol.hpp"
#include "generic_connection_handler.hpp"

using asio::ip::tcp;

namespace StorageAPI {
    class StorageConnectionHandler : public GenericConnectionHandler<StoragePacket>
    {
    private:
        void handle_request(const StoragePacket& request, StoragePacket& response);
        void init_connection(uint16_t id, StoragePacket& response);
        void read(const StoragePacket& request, StoragePacket& response);
        void write(const StoragePacket& request, StoragePacket& response);
    public:
        StorageConnectionHandler() = default;
        StorageConnectionHandler(asio::io_context& context);
        ~StorageConnectionHandler() override = default;
    };
}

#endif