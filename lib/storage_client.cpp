#include "storage_client.hpp"

using namespace StorageAPI;

// private
asio::awaitable<int> StorageClient::read_async(const std::string& path, std::vector<uint8_t>& buffer, size_t size, off_t offset)
{
    co_return 0;
}

asio::awaitable<int> StorageClient::write_async(const std::string& path, const std::vector<uint8_t>& buffer, size_t size, off_t offset)
{
    try 
    {
        StoragePacket request, response;
        request.id = Utils::generate_id();
        request.opcode = OperationCode::to_byte(OperationCode::Type::WRITE);
        request.offset = (uint32_t)offset;
        request.path_len = path.length();
        request.path = Utils::get_byte_array_from_string(path);
        request.data_len = size;
        request.data = buffer;
        co_await send_request_async(request);
        co_await receive_response_async(response);

        if ((response.id != request.id) || response.rescode == ResultCode::Type::ERRMSG)
        {
            if (response.message_len == 0)
                SPDLOG_ERROR("Unknown server error");
            else 
            {
                int error = Utils::get_int_from_byte_array(response.message);
                SPDLOG_ERROR(std::format("Server error: {}", std::strerror(error)));
            }            
            co_return 0;
        }

        co_return size;
    }
    catch (std::exception& e)
    {
        SPDLOG_ERROR(std::format("write_async: {}", e.what()));
        co_return 0;
    }
}

// public
int StorageClient::read(const std::string& path, std::vector<uint8_t>& buffer, size_t size, off_t offset)
{
    std::promise<int> result_promise;
    std::future<int> result_future = result_promise.get_future();
    
    asio::co_spawn(
        context,
        [&]() -> asio::awaitable<void> {
            int result = co_await read_async(path, buffer, size, offset);
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
        return 0;
    }
}

int StorageClient::write(const std::string& path, const std::vector<uint8_t>& buffer, size_t size, off_t offset)
{
    std::promise<int> result_promise;
    std::future<int> result_future = result_promise.get_future();
    
    asio::co_spawn(
        context,
        [&]() -> asio::awaitable<void> {
            int result = co_await write_async(path, buffer, size, offset);
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
        return 0;
    }
}
