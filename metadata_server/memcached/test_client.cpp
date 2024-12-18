#include "cache_api.hpp"
#include <iostream>
#include <unistd.h>

// Alias for readability
using asio::ip::tcp;
using namespace CacheAPI;

// void send_data(tcp::socket& socket, CachePacket& packet) {
//     asio::error_code ec;
//     std::vector<unsigned char> buf;
//     packet.to_buffer(buf);
//     asio::write(socket, asio::buffer(buf), ec);
//     if (ec) {
//         spdlog::error("Failed to send data: {}", ec.message());
//     } else {
//         // spdlog::info("Sent data: \n{}\n\n", packet.to_string());
//     }
// }

// CachePacket receive_data(tcp::socket& socket) {
//     asio::error_code ec;
//     std::vector<unsigned char> buffer(8192); // Adjust buffer size as needed
//     size_t length = socket.read_some(asio::buffer(buffer), ec);
//     if (ec && ec != asio::error::eof) {
//         spdlog::error("Failed to receive data: {}", ec.message());
//         exit(1);
//     } else {
//         CachePacket response(buffer.data(), length);
//         // spdlog::info("Received data: \n{}", response.to_string);
//         return response;
//     }
// }

int main() {
    try {
        ////// LOGGER //////
        spdlog::set_level(spdlog::level::trace); // Set global log level to debug
        spdlog::set_pattern("(%s:%#) [%^%l%$] %v");

        CacheClient client;

        client.connect("127.0.0.1", "8888");

        // // Initialize ASIO context
        // asio::io_context io_context;

        // // Resolve the server address and port
        // tcp::resolver resolver(io_context);
        // tcp::resolver::results_type endpoints = resolver.resolve("0.0.0.0", "8888");

        // // for (int i = 0; i < 3000; i ++)
        // // {
        //     // Create and connect the socket
        //     tcp::socket socket(io_context);
        //     asio::connect(socket, endpoints);

        //     spdlog::info("Connected to server at 0.0.0.0:8888");

        //     // Example: Send and receive data
        //     CachePacket request;
        //     request.id = 0;
        //     request.opcode = OperationCode::Type::SET;

        //     send_data(socket, request);
        //     CachePacket response = receive_data(socket);

        //     spdlog::info("Server response: \n{}", response.to_string());
        //     socket.close();
        // // }

    } catch (std::exception& e) {
        SPDLOG_ERROR("Exception: {}", e.what());
    }
    return 0;
}
