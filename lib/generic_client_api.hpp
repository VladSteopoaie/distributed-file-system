#ifndef GENERIC_CLIENT_API_HPP
#define GENERIC_CLIENT_API_HPP

#define ASIO_STANALONE // non-boost version
#define ASIO_NO_DEPRECATED // no need for deprecated stuff
#define ASIO_HAS_STD_COROUTINE // c++20 coroutines needed
#include <asio.hpp>

#ifndef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif
#include <spdlog/spdlog.h>
#include "net_protocol.hpp"

using asio::ip::tcp;

template <typename Packet>
class GenericClient {
private:

asio::awaitable<size_t> read_socket(tcp::socket& socket, Packet& response)
{
    try {
            std::vector<uint8_t> buffer;
            buffer.resize(64 * 1024); // buffer to store incoming data
            std::vector<uint8_t> packet_buffer;
            packet_buffer.reserve(Packet::max_packet_size);
            
            size_t expected_size = 0;
            size_t bytes_transferred;
            while (true)
            {
                // if (expected_size > 0)
                    // std::cout << "Should expect: " << expected_size - packet_buffer.size() << " bytes" << std::endl; 
                bytes_transferred = co_await socket.async_read_some(asio::buffer(buffer), asio::use_awaitable);
                packet_buffer.insert(packet_buffer.end(), buffer.begin(), buffer.begin() + bytes_transferred);
                
                if (packet_buffer.size() < Packet::header_size)
                continue;
                
                if (!expected_size)
                expected_size = Packet::get_packet_size(packet_buffer.data(), packet_buffer.size());
                
                // std::cout << "Packet size: " << packet_buffer.size() << std::endl;
                // std::cout << "Header size: " << Packet::header_size << std::endl;
                // std::cout << "Expected size: " << expected_size << std::endl;
                if (packet_buffer.size() < expected_size)
                    continue;
                    
                    // co_return packet_buffer.size();
                    break;
                }
                
                response.from_buffer(packet_buffer.data(), packet_buffer.size());
                co_return packet_buffer.size();
                
            }
            catch (std::exception& e)
            {
                throw std::runtime_error(std::format("read_socket: {}", e.what()));
            }
            
            co_return 0;
        }
        
protected:
    int thread_count;
    asio::io_context context;
    tcp::resolver resolver;
    std::string address;
    std::string port;
    std::vector<std::thread> thread_pool; 
    asio::executor_work_guard<asio::io_context::executor_type> work_guard;
        
    asio::awaitable<void> send_request_async(const Packet& request, Packet& response)
    {
        tcp::socket socket = tcp::socket(context);
        try {
            std::vector<uint8_t> buffer; // buffer to store incoming data
            tcp::resolver::results_type endpoints = 
                co_await resolver.async_resolve(address, port, asio::use_awaitable);
            co_await asio::async_connect(socket, endpoints, asio::use_awaitable);
            
            request.to_buffer(buffer);
            co_await asio::async_write(socket, asio::buffer(buffer), asio::use_awaitable);
            
            size_t bytes_received = co_await read_socket(socket, response);
            // socket.async_read_some(asio::buffer(buffer),
            //     [this, self] (std::error_code error, size_t bytes_transferred)
            //     {

            //     });
            // std::cout << Packet::max_packet_size << " vs. " << bytes_received << " vs. " << packet_buffer.size() << std::endl;
            // response.from_buffer(packet_buffer.data(), bytes_received);
            socket.close();
        }
        catch (std::exception& e)
        {
            socket.close();
            throw std::runtime_error(std::format("send_request_async: {}", e.what()));
        }

        co_return;
    }

public:
    GenericClient(const GenericClient&) = delete;
    GenericClient& operator= (const GenericClient&) = delete;

    GenericClient() : GenericClient(1) {} // default thread count 1
    GenericClient(int thread_count)
        : resolver(context) 
        , work_guard(asio::make_work_guard(context))
        , thread_count(thread_count)
    {
        for (int i = 0; i < thread_count; i++)
        {
            thread_pool.emplace_back([this]() {
                try {
                    context.run();
                }
                catch (std::exception& e)
                {
                    SPDLOG_ERROR("Thread error: {}", e.what());
                }
            });
        }
    }

    virtual ~GenericClient() { 
        context.stop();
        for (auto& t : thread_pool)
        {
            if (t.joinable())
                t.join();
        }
    }

    virtual asio::awaitable<void> connect_async(const std::string& address, const std::string& port)
    {
        try {
            this->address = address;
            this->port = port;
            
            // sending an INIT request to see if server is up
            Packet request, response;
            request.id = Utils::generate_id();
            request.opcode = OperationCode::to_byte(OperationCode::Type::INIT);
            co_await send_request_async(request, response);

            if (request.id != response.id || response.id == 0)
            {
                throw std::runtime_error("Invalid packet id.");
            }

            switch (ResultCode::from_byte(response.rescode))
            {
                case ResultCode::Type::SUCCESS:
                    SPDLOG_INFO("Connected successfully!");
                    break;                
                default:
                    throw std::runtime_error(std::format("Response code is not SUCCESS: {}", response.rescode));
            }
        } // try
        catch (std::exception& e)
        {
            throw std::runtime_error(std::format("connect_async: {}", e.what()));
        }

        co_return;
    } // connect_async

    void connect(const std::string& address, const std::string& port)
    {
        std::promise<std::string> result_promise;
        std::future<std::string> result_future = result_promise.get_future();
        
        asio::co_spawn(
            context, 
            [&]() -> asio::awaitable<void> {
                try {
                    co_await connect_async(address, port);
                    result_promise.set_value("");
                }
                catch (std::exception& e)
                {
                    result_promise.set_value(e.what());
                }
                co_return;
            },
            asio::detached
        );
        // context.run();
        // context.restart();

        try {
            // std::cout << "Waiting for connect result..." << std::endl;
            std::string result = result_future.get();
            // std::cout << "Result: " << result << std::endl;

            if (result.length() > 0)
                throw std::runtime_error(result);
        }
        catch (std::exception& e)
        {
            throw std::runtime_error(e.what());
        }
    }
};

#endif