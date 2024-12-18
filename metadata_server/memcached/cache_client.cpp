#include "cache_client.hpp"

using namespace CacheAPI;

//////////////////////////////
// ----[ CLIENT CLASS ]---- //
//////////////////////////////

CacheClient::CacheClient()
    : CacheClient("")
{}

CacheClient::CacheClient(std::string mem_conf_string)
    : socket(context)
    , resolver(context)
    , mem_conf_string(mem_conf_string)
{}

CacheClient::~CacheClient()
{
    memcached_free(mem_client);
    socket.close();
}

asio::awaitable<void> CacheClient::send_request_async(const CachePacket& request)
{
    std::vector<uint8_t> buffer;
    request.to_buffer(buffer);

    try {
        co_await asio::async_write(socket, asio::buffer(buffer), asio::use_awaitable);
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(std::format("send_request_async: {}", e.what()));
    }

    co_return;
}

asio::awaitable<void> CacheClient::receive_response_async(CachePacket& response)
{
    try {
        std::vector<uint8_t> buffer(CachePacket::max_packet_size);
        size_t bytes_received = co_await socket.async_read_some(asio::buffer(buffer), asio::use_awaitable);
        response.from_buffer(buffer.data(), bytes_received);
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(std::format("receive_response_async: {}", e.what()));
    }

    co_return;
}

asio::awaitable<void> CacheClient::connect_async(std::string address, std::string port)
{
    try {
        tcp::resolver::results_type endpoints = 
            co_await resolver.async_resolve(address, port, asio::use_awaitable);
        
        co_await asio::async_connect(socket, endpoints, asio::use_awaitable);
        
        if (mem_conf_string.length() == 0)
        {
            CachePacket request, response;
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
                    SPDLOG_INFO("Success!");
                    std::cout << response.to_string() << std::endl;

                    uint16_t mem_port;
                    
                    // here I should receive the port number of the memcached server
                    // which is 2 bytes long
                    if (response.message_len != 2)
                    {
                        throw std::runtime_error(std::format("Invalid message_len, should be 2, received {}.", response.message_len));
                    }

                    mem_port = (uint16_t)(response.message[0] << 8) + response.message[1];
                    std::cout << mem_port << std::endl;
                    mem_conf_string = std::format("--SERVER={}:{}", address, mem_port);
                    std::cout << mem_conf_string << std::endl;
                    break;
                
                case ResultCode::Type::NOLOCAL:
                    throw std::runtime_error("No local memcached server found, a configuration string is required.");
            }
        }

        // check for a configuration file
        Utils::prepare_conf_string(mem_conf_string);

        mem_client = memcached(mem_conf_string.c_str(), mem_conf_string.length());

        if (mem_client == NULL)
        {
            throw std::runtime_error("Memcached error when initializing connectivity!");
        }
    }
    catch (std::exception& e)
    {
        // throw std::runtime_error(std::format("connect_async: {}", e.what()));
        SPDLOG_ERROR("connect_async: {}", e.what());
    }

    co_return;
}

void CacheClient::connect(std::string address, std::string port)
{
    asio::co_spawn(context, connect_async(address, port), asio::detached);
    context.run();
}
