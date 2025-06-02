#include "storage_client.hpp"

using namespace StorageAPI;

std::ofstream s_log_file("/mnt/tmpfs/s_client.log", std::ios::app);

// private
asio::awaitable<int> StorageClient::read_async(const std::string& path, char* buffer, size_t size, off_t offset)
{
    try 
    {
        StoragePacket request, response;
        request.id = Utils::generate_id();
        request.opcode = OperationCode::to_byte(OperationCode::Type::READ);
        request.offset = (uint32_t)offset;
        request.path_len = path.length();
        request.path = Utils::get_byte_array_from_string(path);
        request.data_len = 4;
        request.data = Utils::get_byte_array_from_int(size);
        
        co_await send_request_async(request, response);
 
        if ((response.id != request.id) || response.rescode == ResultCode::Type::ERRMSG)
        {
            if (response.message_len == 0)
                SPDLOG_ERROR("Unknown server error");
            else 
            {
                std::string error = Utils::get_string_from_byte_array(response.message);
                SPDLOG_ERROR(std::format("Server error: {}", error));
            }            
            co_return 0;
        }

        memcpy(buffer, response.data.data(), response.data_len);

        co_return response.data_len;
    }
    catch (std::exception& e)
    {
        SPDLOG_ERROR(std::format("write_async: {}", e.what()));
        co_return 0;
    }
}


asio::awaitable<int> StorageClient::write_async(
      const std::string& path
    , const char* buffer
    , size_t size
    , off_t offset)
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
        request.data.assign(buffer, buffer + size);
        {
            // Utils::PerformanceTimer timer("StorageClient::read_async", s_log_file);
            co_await send_request_async(request, response);
        }
        // std::cout << response.to_string() << std::endl;
        if ((response.id != request.id) || response.rescode == ResultCode::Type::ERRMSG)
        {
            if (response.message_len == 0)
                SPDLOG_ERROR("Unknown server error");
            else 
            {
                std::string error = Utils::get_string_from_byte_array(response.message);
                SPDLOG_ERROR(std::format("Server error: {}", error));
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

asio::awaitable<int> StorageClient::remove_async(const std::string& path)
{
    try 
    {
        StoragePacket request, response;
        request.id = Utils::generate_id();
        request.opcode = OperationCode::to_byte(OperationCode::Type::RM_FILE);
        request.path_len = path.length();
        request.path = Utils::get_byte_array_from_string(path);
        co_await send_request_async(request, response);

        if ((response.id != request.id) || response.rescode == ResultCode::Type::ERRMSG)
        {
            if (response.message_len == 0)
                SPDLOG_ERROR("Unknown server error");
            else 
            {
                std::string error = Utils::get_string_from_byte_array(response.message);
                SPDLOG_ERROR(std::format("Server error: {}", error));
            }            
            co_return -1;
        }
    }
    catch (std::exception& e)
    {
        SPDLOG_ERROR(std::format("remove_async: {}", e.what()));
        co_return -1;
    }
    co_return 0;
} // remove_async

// public
StorageClient::StorageClient() : StorageClient(1, 4096) {} // default stipe size 4KB
StorageClient::StorageClient(size_t stripe_size) : StorageClient(1, stripe_size) {}
StorageClient::StorageClient(int thread_count, size_t stripe_size) 
    : GenericClient<StoragePacket>(thread_count)
    , stripe_size(stripe_size) 
{
    SPDLOG_INFO("StorageClient:\n\t- stripe size: {}\n\t- thread count: {}", stripe_size, thread_count);   
}

int StorageClient::read(const std::string& path, char* buffer, size_t size, off_t offset)
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

    // context.run();
    // context.restart();

    try {
        return result_future.get();
    }
    catch (...)
    {
        return 0;
    }
}

int StorageClient::write(const std::string& path, const char* buffer, size_t size, off_t offset)
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

    // context.run();
    // context.restar(

    try {
        return result_future.get();
    }
    catch (...)
    {
        return 0;
    }
}

// int StorageClient::write_stripes(const std::string& path, const std::vector<uint8_t>& buffer, size_t size, off_t offset)
// {
//     size_t stripes_num = buffer.size() / stripe_size + (buffer.size() % stripe_size != 0 ? 1 : 0);
//     size_t last_stripe_size = buffer.size() % stripe_size;
//     if (last_stripe_size == 0 && stripes_num > 0) {
//         last_stripe_size = stripe_size;
//     }
    
//     std::vector<std::promise<int>> promises(stripes_num);
//     std::vector<std::future<int>> futures;
    
//     for (size_t i = 0; i < stripes_num; i++) {
//         futures.push_back(promises[i].get_future());
//     }
    
//     for (size_t i = 0; i < stripes_num; i++) {
//         size_t final_size = (i == stripes_num - 1) ? last_stripe_size : stripe_size;
        
//         asio::co_spawn(
//             context,
//             [i, &promises, &path, &buffer, final_size, this]() -> asio::awaitable<void> {
//                 try {
//                     // int result = co_await write_async(path
//                     //     , buffer.begin() + i * stripe_size
//                     //     , buffer.bin() + i stripe_size + final_size
//                     //     , final_size, i * stripe_size);
//                     int result = 1;
//                     promises[i].set_value(result);
//                 } catch (const std::exception& e) {
//                     promises[i].set_exception(std::current_exception());
//                 }
//                 co_return;
//             },
//             asio::detached
//         );
//     }
    
//     // context.run();
//     // context.restart();
    
//     // Sum up all the results
//     int total_bytes_written = 0;
//     try {
//         for (auto& future : futures) {
//             total_bytes_written += future.get();
//         }
//         return total_bytes_written;
//     } catch (...) {
//         return 0;
//     }
// }

int StorageClient::remove(const std::string& path)
{
    std::promise<int> result_promise;
    std::future<int> result_future = result_promise.get_future();
    
    asio::co_spawn(
        context,
        [&]() -> asio::awaitable<void> {
            int res = co_await remove_async(path);
            result_promise.set_value(res);
            co_return;
        },
        asio::detached
    );

    try {
        return result_future.get();
    }
    catch (...)
    {
        return -1;
    }
}