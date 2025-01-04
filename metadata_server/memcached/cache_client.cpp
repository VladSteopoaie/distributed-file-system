#include "cache_client.hpp"

using namespace CacheAPI;

CacheClient::CacheClient()
    : CacheClient("")
{}

CacheClient::CacheClient(std::string mem_conf_string)
    : socket(context)
    , resolver(context)
    , mem_conf_string(mem_conf_string)
    , mem_client(NULL)
{}

CacheClient::~CacheClient()
{
    if (mem_client != NULL)
        memcached_free(mem_client);
    socket.close();
}

asio::awaitable<void> CacheClient::send_request_async(const CachePacket& request)
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

asio::awaitable<void> CacheClient::receive_response_async(CachePacket& response)
{
    SPDLOG_DEBUG("Receiving packet.");
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

        this->address = address;
        this->port = port;
        
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
                    uint16_t mem_port;
                    
                    // here I should receive the port number of the memcached server
                    // which is 2 bytes long
                    if (response.message_len != 2)
                    {
                        throw std::runtime_error(std::format("Invalid message_len, should be 2, received {}.", response.message_len));
                    }

                    mem_port = (uint16_t)(response.message[0] << 8) + response.message[1];
                    mem_conf_string = std::format("--SERVER={}:{}", address, mem_port);
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
        SPDLOG_INFO("Connected successfully!");
    }
    catch (std::exception& e)
    {
        SPDLOG_ERROR("connect_async: {}", e.what());
    }

    co_return;
}

asio::awaitable<void> CacheClient::set_async(std::string key, std::string value, uint32_t time, uint8_t flags)
{
    try {
        CachePacket request, response;
        request.id = Utils::generate_id();
        request.opcode = OperationCode::to_byte(OperationCode::Type::SET);
        request.time = time;
        request.flags = flags;
        request.key_len = key.length();
        request.value_len = value.length();
        request.key = Utils::get_byte_array_from_string(key);
        request.value = Utils::get_byte_array_from_string(value);
        co_await send_request_async(request);
        co_await receive_response_async(response);
            
        if ((response.id != request.id) || response.rescode == ResultCode::Type::ERRMSG)
        {
            if (response.message_len == 0)
                SPDLOG_ERROR("Unknown server error");
            else 
                SPDLOG_ERROR(std::format("Server error: {}", Utils::get_string_from_byte_array(response.message)));
            
            co_return;
        }

        if (response.rescode == ResultCode::Type::SUCCESS)
            SPDLOG_INFO("Stored!");
        else 
            SPDLOG_ERROR("Invalid packet from server.");
    }
    catch (std::exception& e)
    {
        SPDLOG_ERROR(std::format("set_async: {}", e.what()));
    }

    co_return;
}


asio::awaitable<void> CacheClient::set_async(std::string key, std::string value)
{
    co_await set_async(key, value, 0, 0);
    co_return;
}

asio::awaitable<std::string> CacheClient::get_async(std::string key)
{
    try {
        CachePacket request, response;
        request.id = Utils::generate_id();
        request.opcode = OperationCode::to_byte(OperationCode::Type::GET);

        request.key_len = key.length();
        request.key = Utils::get_byte_array_from_string(key);

        co_await send_request_async(request);
        co_await receive_response_async(response);

        if ((response.id != request.id) || response.rescode == ResultCode::Type::ERRMSG)
        {
            if (response.message_len == 0)
                SPDLOG_ERROR("Unknown server error");
            else 
                SPDLOG_ERROR(std::format("Server error: {}", Utils::get_string_from_byte_array(response.message)));
            
            co_return "";
        }
        co_return Utils::get_string_from_byte_array(response.value);
    }
    catch (std::exception& e)
    {
        SPDLOG_ERROR(std::format("get_async: {}", e.what()));
    }

    co_return "";
}

void CacheClient::set(std::string key, std::string value, uint32_t time, uint8_t flags)
{
    asio::co_spawn(context, set_async(key, value, time, flags), asio::detached);
    context.run();
    context.restart();
}

void CacheClient::set(std::string key, std::string value)
{
    set(key, value, 0, 0);
}

std::string CacheClient::get(std::string key)
{
    std::promise<std::string> result_promise;
    std::future<std::string> result_future = result_promise.get_future();
    
    asio::co_spawn(
        context,
        [&]() -> asio::awaitable<void> {
            std::string result = co_await get_async(key);
            result_promise.set_value(result);
            co_return;
        },
        asio::detached
    );

    context.run();
    context.restart();

    try {
        return result_future.get();
    }
    catch (...)
    {
        return "";
    }
}


void CacheClient::connect(std::string address, std::string port)
{
    asio::co_spawn(context, connect_async(address, port), asio::detached);
    context.run();
    context.restart();
}
