#include "storage_server.hpp"

using namespace StorageAPI;

template class GenericServer<StorageConnectionHandler>;
// StorageServer::StorageServer(std::vector<Utils::ConnectionInfo<StoragePacket>>&& connections)
//         : StorageServer(1, 4096, std::move(connections)) {}

// StorageServer::StorageServer(int thread_count, std::vector<Utils::ConnectionInfo<StoragePacket>>&& connections)
//     : StorageServer(thread_count, 4096, std::move(connections)) {}

StorageServer::StorageServer(int thread_count, int stripe_size, std::vector<Utils::ConnectionInfo<StoragePacket>> connections)
    : GenericServer<StorageConnectionHandler>::GenericServer(thread_count)
    , stripe_size(stripe_size)
    , connections(connections)
{}

void StorageServer::init() {
    StoragePacket init_packet;
    std::vector<uint8_t> init_data, response_data[this->connections.size()];
    std::future<size_t> futures[this->connections.size()];
    init_packet.opcode = OperationCode::to_byte(OperationCode::Type::INIT);
    init_packet.message_len = 4;
    init_packet.message = Utils::get_byte_array_from_int(stripe_size);

    for (int i = 0; i < this->connections.size(); i++)
    {
        init_packet.id = Utils::generate_id();
        init_packet.to_buffer(init_data);
        
        futures[i] = asio::co_spawn(
            context,
            this->connections[i].send_receive_data_async(context, init_data, response_data[i]),
            asio::use_future
        );
        SPDLOG_INFO("Initialized connection with {}:{}", this->connections[i].address, this->connections[i].port);
    }

    StoragePacket response_packet;
    for (int i = 0; i < this->connections.size(); i++)
    {
        futures[i].wait();
        response_packet.from_buffer(response_data[i].data(), response_data[i].size());
        if (response_packet.rescode != ResultCode::Type::SUCCESS)
        {
            SPDLOG_ERROR("Failed to initialize connection with {}:{}. Error: {}", this->connections[i].address, this->connections[i].port, Utils::get_string_from_byte_array(response_packet.message));
            throw std::runtime_error("Failed to initialize connection with " + this->connections[i].address + ":" + std::to_string(this->connections[i].port));
        }
    }
}

void StorageServer::run(uint16_t port) {
    GenericServer<StorageConnectionHandler>::run(port, stripe_size, connections);
}

StorageServer::~StorageServer()
{
    SPDLOG_INFO("Exiting Storage Server.");
}