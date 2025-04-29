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
protected:
    asio::io_context context;
    tcp::socket socket;
    tcp::resolver resolver;
    std::string address;
    std::string port;

    asio::awaitable<void> send_request_async(const Packet& request)
    {
        try {
            tcp::resolver::results_type endpoints = 
                co_await resolver.async_resolve(address, port, asio::use_awaitable);
            
            co_await asio::async_connect(socket, endpoints, asio::use_awaitable);
            
            SPDLOG_DEBUG("Sending packet.");
            std::vector<uint8_t> buffer;
            request.to_buffer(buffer);

            co_await asio::async_write(socket, asio::buffer(buffer), asio::use_awaitable);
        }
        catch (std::exception& e)
        {
            throw std::runtime_error(std::format("send_request_async: {}", e.what()));
        }

        co_return;
    }

    asio::awaitable<void> receive_response_async(Packet& response)
    {
        SPDLOG_DEBUG("Receiving packet.");
        try {
            std::vector<uint8_t> buffer(Packet::max_packet_size);
            size_t bytes_received = co_await socket.async_read_some(asio::buffer(buffer), asio::use_awaitable);
            response.from_buffer(buffer.data(), bytes_received);
        }
        catch (std::exception& e)
        {
            throw std::runtime_error(std::format("receive_response_async: {}", e.what()));
        }

        co_return;
    }

public:
    GenericClient(const GenericClient&) = delete;
    GenericClient& operator= (const GenericClient&) = delete;

    GenericClient()
        : socket(context)
        , resolver(context) 
    {}
    virtual ~GenericClient() { socket.close(); }

    virtual asio::awaitable<void> connect_async(const std::string& address, const std::string& port)
    {
        try {
            this->address = address;
            this->port = port;
            
            // sending an INIT request to see if server is up
            Packet request, response;
            request.id = Utils::generate_id();
            request.opcode = OperationCode::to_byte(OperationCode::Type::INIT);
            co_await send_request_async(request);
            co_await receive_response_async(response);

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
        context.run();
        context.restart();

        try {
            std::string result = result_future.get();

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