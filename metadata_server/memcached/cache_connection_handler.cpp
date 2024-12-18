#include "cache_connection_handler.hpp"

using namespace CacheAPI;

////////////////////////////////////
// ----[ CONNECTION HANDLER ]---- //
////////////////////////////////////

// private
void CacheConnectionHandler::handle_error(std::string error)
{
    SPDLOG_ERROR(error);
}

int CacheConnectionHandler::set_cache_object(std::string key, std::string value, time_t expiration, uint32_t flags)
{
    memcached_return_t result = memcached_set(mem_client, 
            key.c_str(), key.size(), 
            value.c_str(), value.size(),
            expiration, flags);

    if (result != MEMCACHED_SUCCESS)
    {
        throw std::runtime_error(std::format("set_cache_object: {}", memcached_strerror(mem_client, result)));
    }

    return 0;
}

int CacheConnectionHandler::set_cache_object(std::string key, std::string value)
{
    return set_cache_object(key, value, 0, 0);
}

std::string CacheConnectionHandler::get_cache_object(std::string key)
{
    memcached_return_t error;
    char* result = memcached_get(mem_client, key.c_str(), key.length(), NULL, NULL, &error);

    if (result == NULL)
    {
        throw std::runtime_error(std::format("get_cache_object: {}", memcached_strerror(mem_client, error)));
    }

    return result;
}

// public
CacheConnectionHandler::CacheConnectionHandler(asio::io_context& context, memcached_st* mem_client, uint16_t mem_port)
    : context(context)
    , socket(context)
    , mem_client(mem_client)
    , mem_port(mem_port)
{}

CacheConnectionHandler::~CacheConnectionHandler() {
    socket.close();
    std::cout << "Connection ended!" << std::endl;
}

tcp::socket& CacheConnectionHandler::get_socket() { return socket; }


void CacheConnectionHandler::start() 
{
    tcp::endpoint remote_endpoint =  socket.remote_endpoint();
    SPDLOG_INFO("New connection from {}:{}.",
        remote_endpoint.address().to_string(), remote_endpoint.port());
    read_socket_async();
}

void CacheConnectionHandler::read_socket_async()
{
    auto self(shared_from_this()); // used to keep the connection alive

    buffer.resize(CachePacket::max_packet_size);
    socket.async_read_some(asio::buffer(buffer),
        [this, self] (std::error_code error, size_t bytes_transferred)
        {
            try {
                if (error)
                {
                    throw std::runtime_error(error.message());
                }

                CachePacket request(buffer.data(), bytes_transferred);
                
                CachePacket response;
                handle_request(request, response);

                response.to_buffer(buffer);
                self->write_socket_async();
            }
            catch (std::exception& e)
            {
                self->handle_error(std::format("read_socket_async: {}", e.what()));
            }

        });
}

void CacheConnectionHandler::write_socket_async()
{
    auto self(shared_from_this()); // used to keep the connection alive

    asio::async_write(socket, asio::buffer(buffer),
        [this, self] (std::error_code error, size_t bytes_transferred)
        {
            if (error)
            {
                self->handle_error(std::format("write_socket_async: {}", error.message()));
                return;
            }

            SPDLOG_INFO("Connection handled successfully!");
        });
}

void CacheConnectionHandler::handle_request(const CachePacket& request, CachePacket& response)
{
    if (request.id == 0)
    {
        response.rescode = ResultCode::to_byte(ResultCode::Type::INVPKT);
        return;
    }

    response.id = request.id; 
    response.opcode = request.opcode;

    switch (OperationCode::from_byte(request.opcode))
    {
        case OperationCode::Type::NOP:
            response.rescode = ResultCode::to_byte(ResultCode::Type::SUCCESS);
            break;
        case OperationCode::Type::GET:
            get_local_object(request, response);
            break;
        case OperationCode::Type::SET:
            set_local_object(request, response);
            break;
        case OperationCode::Type::INIT:
            init_connection(response);
            break;
        default:
            response.rescode = ResultCode::to_byte(ResultCode::Type::INVOP);
            break;
    }

}

void CacheConnectionHandler::set_local_object(const CachePacket& request, CachePacket& response)
{
    SPDLOG_INFO("In set_local_object!");
    std::cout << request.to_string() << std::endl;
}

void CacheConnectionHandler::get_local_object(const CachePacket& request, CachePacket& response)
{
    SPDLOG_INFO("In get_local_object!");
    std::cout << request.to_string() << std::endl;
}

void CacheConnectionHandler::init_connection(CachePacket& response)
{
    std::cout << mem_port << std::endl;
    if (mem_port <= 0)
    {
        response.rescode = ResultCode::to_byte(ResultCode::Type::NOLOCAL);
        return;
    }

    response.rescode = ResultCode::Type::SUCCESS;
    response.message_len = 2; // sending the port which has 2 bytes
    response.message.clear();
    response.message.push_back(mem_port >> 8);
    response.message.push_back(mem_port & 0xFF);
}
