#ifndef CONNECTION_HANDLER_HPP
#define CONNECTION_HANDLER_HPP

#define ASIO_STANALONE // non-boost version
#define ASIO_NO_DEPRECATED // no need for deprecated stuff
#define ASIO_HAS_STD_COROUTINE // c++20 coroutines needed
#include <asio.hpp>

#ifndef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif
#include <spdlog/spdlog.h>

using asio::ip::tcp;

template <typename Packet>
class GenericConnectionHandler : public std::enable_shared_from_this<GenericConnectionHandler<Packet>>
{
protected:
    asio::io_context& context;
    tcp::socket socket;
    std::vector<uint8_t> buffer = std::vector<uint8_t>(Packet::max_packet_size); // buffer to store incoming data
    std::vector<uint8_t> packet_buffer; // buffer for dynamic buffering
    size_t expected_size = 0; // expected packet bytes to be received

    virtual void handle_request(const Packet& request, Packet& response) = 0;
    
    void read_socket_async()
    {
        auto self = this->shared_from_this(); // used to keep the connection alive
        socket.async_read_some(asio::buffer(buffer),
            [this, self] (std::error_code error, size_t bytes_transferred)
            {
                try {
                    if (error)
                    {
                        throw std::runtime_error(error.message());
                    }

                    packet_buffer.insert(packet_buffer.end(), buffer.begin(), buffer.begin() + bytes_transferred);

                    while (true)
                    {
                        if (packet_buffer.size() < Packet::header_size)
                            break;

                        if (!expected_size)
                            expected_size = Packet::get_packet_size(packet_buffer.data(), packet_buffer.size());

                        SPDLOG_INFO("transfered: {}", bytes_transferred);
                        SPDLOG_INFO("current size: {}", packet_buffer.size());
                        SPDLOG_INFO("expected: {}", expected_size);
                        
                        if (packet_buffer.size() < expected_size)
                            break;
                        
                        Packet request(packet_buffer.data(), packet_buffer.size());

                        Packet response;
                        handle_request(request, response);

                        response.to_buffer(buffer);
                        self->write_socket_async();
                        return;
                    }

                    self->read_socket_async();
                }
                catch (std::exception& e)
                {
                    SPDLOG_ERROR(std::format("read_socket_async: {}", e.what()));
                }
            });
    }

    void write_socket_async()
    {
        auto self = this->shared_from_this(); // used to keep the connection alive

        asio::async_write(socket, asio::buffer(buffer),
            [this, self] (std::error_code error, size_t bytes_transferred)
            {
                if (error)
                {
                    SPDLOG_ERROR(std::format("write_socket_async: {}", error.message()));
                    return;
                }

                SPDLOG_INFO("Connection handled successfully!");
            });
    }

public:
    GenericConnectionHandler(asio::io_context& context)
        : context(context)
        , socket(context)
    {}
    virtual ~GenericConnectionHandler() { 
        socket.close(); 
        packet_buffer.clear();
        buffer.clear();
    }

    tcp::socket& get_socket() { return socket; }


    void start() 
    {
        tcp::endpoint remote_endpoint = socket.remote_endpoint();
        SPDLOG_INFO("New connection from {}:{}.",
            remote_endpoint.address().to_string(), remote_endpoint.port());
        read_socket_async();
    }
};

#endif