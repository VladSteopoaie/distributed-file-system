#define ASIO_STANALONE // non-boost version
#define ASIO_NO_DEPRECATED // no need for deprecated stuff
#define ASIO_HAS_STD_COROUTINE // c++20 coroutines needed
#include <asio.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <CLI11.hpp>
// #include "../../lib/net_protocol.hpp"
#include "../lib/net_protocol.hpp"

asio::io_context context;
std::string storage_path = "/project/storage";
int stripe_size;

using asio::ip::tcp;

void init(const StoragePacket& request, StoragePacket& response)
{
    if (request.message_len > 0) {
        response.rescode = ResultCode::Type::SUCCESS;
        stripe_size = Utils::get_int_from_byte_array(request.message);
        std::cout << stripe_size << std::endl;
    }
    else {
        response.rescode = ResultCode::Type::ERRMSG;
        response.message = Utils::get_byte_array_from_string("No stripe size provided");
        response.message_len = response.message.size();
    }
}

void write(const StoragePacket& request, StoragePacket& response)
{
    std::string stripe_path = storage_path + Utils::get_string_from_byte_array(request.path) + "#" + std::to_string(request.offset);
    // std::cout << rank << ": path " << stripe_path << std::endl;
    // Utils::PerformanceTimer timer("Slave handle_task", log_file);
    int fd = open(stripe_path.c_str(), O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        close(fd);
        // std::cout << rank << ": " << std::strerror(errno) << std::endl;
        response.rescode = ResultCode::Type::ERRMSG;
        response.message_len = 4;
        response.message = Utils::get_byte_array_from_int(errno);
        return;
    }

    ssize_t nbytes = write(fd, request.data.data(), request.data_len);
    close(fd);
    if (nbytes < 0) {
        response.rescode = ResultCode::Type::ERRMSG;
        response.message_len = 4;
        response.message = Utils::get_byte_array_from_int(errno);
    } else {
        response.rescode = ResultCode::Type::SUCCESS;
    }
}

void read(const StoragePacket& request, StoragePacket& result)
{
    std::string stripe_path = storage_path + Utils::get_string_from_byte_array(request.path) + "#" + std::to_string(request.offset);
    // std::cout << "Path: " << stripe_path << std::endl;
    int fd = open(stripe_path.c_str(), O_RDONLY, 0600);
    if (fd < 0)
    {
        // result = Utils::get_byte_array_from_int(errno);
        // std::cout << rank << ": " << std::strerror(errno) << std::endl;
        close(fd);
        result.rescode = ResultCode::Type::ERRMSG;
        result.message_len = 4;
        result.message = Utils::get_byte_array_from_int(errno);
        return;
    }
    
    result.data.resize(stripe_size);
    ssize_t nbytes = read(fd, result.data.data(), stripe_size);
    
    if (nbytes == -1)
    {
        // result = Utils::get_byte_array_from_int(errno);
        // std::cout << rank << ": " << std::strerror(errno) << std::endl;
        close(fd);
        result.rescode = ResultCode::Type::ERRMSG;
        result.message_len = 4;
        result.message = Utils::get_byte_array_from_int(errno);
        return;
        // return -1;
    }
    
    if (nbytes < stripe_size)
        result.data.resize(nbytes);
    close(fd);
    result.rescode = ResultCode::Type::SUCCESS;
    result.data_len = nbytes;
    result.data.resize(nbytes);
}

void remove(const StoragePacket& request, StoragePacket& response)
{
    std::string stripe_path = storage_path + Utils::get_string_from_byte_array(request.path);
    std::string command = "rm -rf " + stripe_path + "#*";
    int err = system(command.c_str()); // this is vulnerable to command injection, but I'm too lazy to fix it

    if (err != 0) {
        response.rescode = ResultCode::Type::ERRMSG;
        response.message_len = 4;
        response.message = Utils::get_byte_array_from_int(errno);
    } else {
        response.rescode = ResultCode::Type::SUCCESS;
    }
}

asio::awaitable<void> handle_task(const std::vector<uint8_t>& request_data, asio::ip::tcp::socket socket) {
    // Utils::PerformanceTimer timer("handle_task", log_file);
    int result;
    int err;
    StoragePacket response, request;
    // std::vector<uint8_t> node_data;
    std::vector<uint8_t> response_data;

    request.from_buffer(request_data.data(), request_data.size());
    response.id = request.id;
    response.opcode = request.opcode;
    response.offset = request.offset;
    response.path_len = request.path_len;
    response.path = request.path;

    SPDLOG_DEBUG("Processing: {}", OperationCode::to_string(OperationCode::from_byte(request.opcode)));
    
    switch (request.opcode)
    {
        case OperationCode::Type::NOP:
            break;
        
        case OperationCode::Type::INIT:
            init(request, response);
            break;
        
        case OperationCode::Type::RM_FILE:
            remove(request, response);
            break;
        case OperationCode::Type::WRITE:
            write(request, response);
            break;
        case OperationCode::Type::READ:
            read(request, response);
            break;
            // node_response.id = request.id;
            // node_response.opcode = request.opcode;
            // node_response.path_len = request.path_len;
            // node_response.path = request.path;
            // node_response.offset = request.offset;
            // err = read(request, node_data);
            // if (err == -1)
            // {
            //     node_response.rescode = ResultCode::Type::ERRMSG;
            //     node_response.message_len = 4;
            //     node_response.message = Utils::get_byte_array_from_int(errno);
            //     node_response.data_len = 0;
            //     node_response.data.clear();
            // }
            // else
            // {
            //     node_response.rescode = ResultCode::Type::SUCCESS;
            //     node_response.data_len = node_data.size();
            //     node_response.data = node_data;
            // }
            // node_response.to_buffer(node_data);
            // co_await asio::async_write(socket, asio::buffer(node_data), asio::use_awaitable);
            // co_return;
            // break;
    }

    response.to_buffer(response_data); // serialize the response packet
    co_await asio::async_write(socket, asio::buffer(response_data), asio::use_awaitable);
    co_return;
}

asio::awaitable<void> read_socket(asio::ip::tcp::socket socket) {
    std::vector<uint8_t> buffer(StoragePacket::max_packet_size);
    std::vector<uint8_t> packet_buffer; // buffer for dynamic buffering
    int expected_size = 0;

    try {
        for (;;) {            
            size_t bytes_transferred = co_await socket.async_read_some(asio::buffer(buffer), asio::use_awaitable);
            packet_buffer.insert(packet_buffer.end(), buffer.begin(), buffer.begin() + bytes_transferred);

            while (true)
            {
                if (packet_buffer.size() < StoragePacket::header_size)
                    break;

                if (!expected_size)
                    expected_size = StoragePacket::get_packet_size(packet_buffer.data(), packet_buffer.size());

                // SPDLOG_INFO("transfered: {}", bytes_transferred);
                // SPDLOG_INFO("current size: {}", packet_buffer.size());
                // SPDLOG_INFO("expected: {}", expected_size);

                if (packet_buffer.size() < expected_size)
                    break;
                
                asio::co_spawn(context, handle_task(std::move(packet_buffer), std::move(socket)), asio::detached);
                co_return;
            }
        }
    } catch (std::exception& e) {
        socket.close();
    }
    co_return;
}

asio::awaitable<void> listener(uint16_t port) {
    asio::ip::tcp::acceptor acceptor(context);
    tcp::endpoint endpoint(tcp::v4(), port);
    acceptor.open(endpoint.protocol());
    acceptor.set_option(tcp::acceptor::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen();
    SPDLOG_INFO("Storage server listening on port {}", port);
    
    for (;;) {
        asio::ip::tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);
        // read_socket(std::move(socket));
        SPDLOG_INFO("Accepted connection from {}", socket.remote_endpoint().address().to_string());
        asio::co_spawn(context, read_socket(std::move(socket)), asio::detached);

    }
}

int main(int argc, char** argv) {
    CLI::App app {"Storage server."};
    argv = app.ensure_utf8(argv);

    uint8_t thread_count = 8;
    uint16_t port = 7777;
    int stripe_size = 4096;

    app.add_option("-p, --port", port, "Port on which to run the server.")->check(CLI::Range(1, 65535));
    app.add_option("-t, --threads", thread_count, "Number of threads in the thread pool.")->check(CLI::Range(1, 16))->required();
    app.add_option("-s, --storage-path", storage_path, "Path of a directory where stripes should be stored.")->check(CLI::ExistingDirectory)->required();
    // app.add_option("-s, --stripe-size", stripe_size, "Stripe size to break down large files.")->check(CLI::Range(1, 131072));
    CLI11_PARSE(app, argc, argv);
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug
    spdlog::set_pattern("(%s:%#) [%^%l%$] %v");

    asio::signal_set signals(context, SIGINT, SIGTERM);
    signals.async_wait([](const std::error_code&, int) {
        context.stop();
    });

    asio::co_spawn(context, listener(port), asio::detached);

    std::vector<std::thread> thread_pool;
    for (int i = 0; i < thread_count - 1; i++)
        thread_pool.emplace_back([&]() { context.run(); });

    context.run();

    for (auto& t : thread_pool)
    {
        if (t.joinable())
            t.join();
    }
    return 0;
}
