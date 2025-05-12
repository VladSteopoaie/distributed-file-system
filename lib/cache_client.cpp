#include "cache_client.hpp"

using namespace CacheAPI;

// private
std::string CacheClient::get_memcached_object(const std::string& key)
{
    memcached_return_t error;
    char* result = memcached_get(mem_client, key.c_str(), key.length(), NULL, NULL, &error);

    if (result == NULL)
    {
        SPDLOG_DEBUG(std::format("get_cache_object: {}", memcached_strerror(mem_client, error)));
        return "";
    }

    return result;
}

asio::awaitable<int> CacheClient::set_async(const std::string& key, const std::string& value, uint32_t time, uint8_t flags, bool is_file)
{
    try {
        CachePacket request, response;
        request.id = Utils::generate_id();
        if (is_file)
            request.opcode = OperationCode::to_byte(OperationCode::Type::SET_FILE);
        else 
            request.opcode = OperationCode::to_byte(OperationCode::Type::SET_DIR);
        request.time = time;
        request.flags = flags;
        request.key_len = key.length();
        request.value_len = value.length();
        request.key = Utils::get_byte_array_from_string(key);
        request.value = Utils::get_byte_array_from_string(value);
        co_await send_request_async(request, response);
            
        if ((response.id != request.id) || response.rescode == ResultCode::Type::ERRMSG)
        {
            int error;
            if (response.message_len == 0)
            {
                SPDLOG_ERROR("Unknown server error");
                error = -1;
            }
            else 
            {
                int error = Utils::get_int_from_byte_array(response.message);
                SPDLOG_ERROR(std::format("Server error: {}", std::strerror(error)));
                if (error == 0) error = -1;
            }             

            co_return error;
        }

        if (response.rescode == ResultCode::Type::SUCCESS)
        {
            SPDLOG_INFO("Stored!");
            co_return 0;
        } 
        
        SPDLOG_ERROR("Invalid packet from server.");
        co_return -1;
    } // try
    catch (std::exception& e)
    {
        SPDLOG_ERROR(std::format("set_async: {}", e.what()));
        co_return -1;
    }

} // set_async

int CacheClient::set(const std::string& key, const std::string& value, uint32_t time, uint8_t flags, bool is_file)
{
    std::promise<int> result_promise;
    std::future<int> result_future = result_promise.get_future();

    asio::co_spawn(
        context,
        [&]() -> asio::awaitable<void> {
            int result = co_await set_async(key, value, time, flags, is_file);
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
        return -1;
    }
}

int CacheClient::set(const std::string& key, const std::string& value, bool is_file)
{
    return set(key, value, 0, 0, is_file);
}

asio::awaitable<std::string> CacheClient::get_async(const std::string& key, bool is_file)
{
    try {
        std::string mem_value = get_memcached_object(key);
        if (mem_value.length() > 0)
            co_return mem_value;

        CachePacket request, response;
        request.id = Utils::generate_id();

        if (is_file)
            request.opcode = OperationCode::to_byte(OperationCode::Type::GET_FILE);
        else
            request.opcode = OperationCode::to_byte(OperationCode::Type::GET_DIR);

        request.key_len = key.length();
        request.key = Utils::get_byte_array_from_string(key);

        co_await send_request_async(request, response);

        if ((response.id != request.id) || response.rescode == ResultCode::Type::ERRMSG)
        {
            if (response.message_len == 0)
                SPDLOG_ERROR("Unknown server error");
            else 
            {
                int error = Utils::get_int_from_byte_array(response.message);
                SPDLOG_ERROR(std::format("Server error: {}", std::strerror(error)));
            }            
            co_return "";
        }
        co_return Utils::get_string_from_byte_array(response.value);
    } // try
    catch (std::exception& e)
    {
        SPDLOG_ERROR(std::format("get_async: {}", e.what()));
    }

    co_return "";
} // get_async

std::string CacheClient::get(const std::string& key, bool is_file)
{
    std::promise<std::string> result_promise;
    std::future<std::string> result_future = result_promise.get_future();
    
    asio::co_spawn(
        context,
        [&]() -> asio::awaitable<void> {
            std::string result = co_await get_async(key, is_file);
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
        return "";
    }
}

asio::awaitable<int> CacheClient::remove_async(const std::string& key, bool is_file)
{
    try {
        CachePacket request, response;
        request.id = Utils::generate_id();
        if (is_file)
            request.opcode = OperationCode::to_byte(OperationCode::Type::RM_FILE);
        else 
            request.opcode = OperationCode::to_byte(OperationCode::Type::RM_DIR);
        request.key_len = key.length();
        request.key = Utils::get_byte_array_from_string(key);
        co_await send_request_async(request, response);
            
        if ((response.id != request.id) || response.rescode == ResultCode::Type::ERRMSG)
        {
            int error;
            if (response.message_len == 0)
            {
                SPDLOG_ERROR("Unknown server error");
                error = -1;
            }
            else 
            {
                int error = Utils::get_int_from_byte_array(response.message);
                SPDLOG_ERROR(std::format("Server error: {}", std::strerror(error)));
                if (error == 0) error = -1;
            }             

            co_return error;
        }

        if (response.rescode == ResultCode::Type::SUCCESS)
        {
            SPDLOG_INFO("Removed!");
            co_return 0;
        } 
        
        SPDLOG_ERROR("Invalid packet from server.");
        co_return -1;
    }
    catch (std::exception& e)
    {
        SPDLOG_ERROR(std::format("remove_async: {}", e.what()));
        co_return -1;
    }
} // remove_async

int CacheClient::remove(const std::string& key, bool is_file)
{
    std::promise<int> result_promise;
    std::future<int> result_future = result_promise.get_future();

    asio::co_spawn(
        context,
        [&]() -> asio::awaitable<void> {
            int result = co_await remove_async(key, is_file);
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
        return -1;
    }
}

asio::awaitable<int> CacheClient::update_async(const std::string& key, const UpdateCommand& command)
{
    try {
        CachePacket request, response;
        request.id = Utils::generate_id();
        request.opcode = OperationCode::to_byte(OperationCode::Type::UPDATE);
        request.key_len = key.length();
        request.key = Utils::get_byte_array_from_string(key);
        command.to_buffer(request.value);
        request.value_len = request.value.size();

        co_await send_request_async(request, response);
        // co_await receive_onse_anc(response);
            
        if ((response.id != request.id) || response.rescode == ResultCode::Type::ERRMSG)
        {
            int error;
            if (response.message_len == 0)
            {
                SPDLOG_ERROR("Unknown server error");
                error = -1;
            }
            else 
            {
                int error = Utils::get_int_from_byte_array(response.message);
                SPDLOG_ERROR(std::format("Server error: {}", std::strerror(error)));
                if (error == 0) error = -1;
            }             

            co_return error;
        }

        if (response.rescode == ResultCode::Type::SUCCESS)
        {
            SPDLOG_INFO("Updated!");
            co_return 0;
        } 
        
        SPDLOG_ERROR("Invalid packet from server.");
        co_return -1;
    } // try
    catch (std::exception& e)
    {
        SPDLOG_ERROR(std::format("update_async: {}", e.what()));
        co_return -1;
    }
} // update_async

int CacheClient::update(const std::string& key, const UpdateCommand& command)
{
    std::promise<int> result_promise;
    std::future<int> result_future = result_promise.get_future();

    asio::co_spawn(
        context,
        [&]() -> asio::awaitable<void> {
            int result = co_await update_async(key, command);
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
    catch (std::exception& e)
    {
        SPDLOG_ERROR("update: {}", e.what());
        return -1;
    }
}

// public
CacheClient::CacheClient()
    : CacheClient(1, "")
{}

CacheClient::CacheClient(const std::string& mem_conf_string)
    : CacheClient(1, mem_conf_string)
{}

CacheClient::CacheClient(int thread_count, const std::string& mem_conf_string)
    : GenericClient<CachePacket>(thread_count)
    , mem_conf_string(mem_conf_string)
    , mem_client(NULL)
{
    SPDLOG_INFO("CacheClient:\n\t- memcached config: {}\n\t- thread count: {}", mem_conf_string, thread_count);
}


CacheClient::~CacheClient()
{
    if (mem_client != NULL)
        memcached_free(mem_client);
}

asio::awaitable<void> CacheClient::connect_async(const std::string& address, const std::string& port)
{
    try {
        this->address = address;
        this->port = port;
        
        if (mem_conf_string.length() == 0)
        {
            CachePacket request, response;
            request.id = Utils::generate_id();
            request.opcode = OperationCode::to_byte(OperationCode::Type::INIT);

            co_await send_request_async(request, response);
            // co_await receie_response_async(response);
            
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
        } // if

        // check for a configuration file
        Utils::prepare_conf_string(mem_conf_string);

        mem_client = memcached(mem_conf_string.c_str(), mem_conf_string.length());

        if (mem_client == NULL)
        {
            throw std::runtime_error("Memcached error when initializing connectivity!");
        }
        SPDLOG_INFO("Connected successfully!");
    } // try
    catch (std::exception& e)
    {
        throw std::runtime_error(std::format("connect_async: {}", e.what()));
    }

    co_return;
} // connect_async
 
int CacheClient::set_file(const std::string& key, const std::string& value)
{
    return set(key, value, true);
}

int CacheClient::set_dir(const std::string& key, const std::string& value)
{
    return set(key, value, false);
}

std::string CacheClient::get_file(const std::string& key)
{
    return get(key, true);
}

std::string CacheClient::get_dir(const std::string& key)
{
    return get(key, false);
}

int CacheClient::remove_file(const std::string& key)
{
    return remove(key, true);
}

int CacheClient::remove_dir(const std::string& key)
{
    return remove(key, false);
}

int CacheClient::chmod(const std::string& key, mode_t new_mode)
{
    UpdateCommand command;
    command.opcode = UpdateCode::to_byte(UpdateCode::Type::CHMOD);
    command.argv.push_back(Utils::get_byte_array_from_int(new_mode));
    command.argc = 1;
    return update(key, command);
}

int CacheClient::chown(const std::string& key, uid_t new_uid, gid_t new_gid)
{
    UpdateCommand command;
    command.opcode = UpdateCode::to_byte(UpdateCode::Type::CHOWN);
    command.argv.push_back(Utils::get_byte_array_from_int(new_uid));
    command.argv.push_back(Utils::get_byte_array_from_int(new_gid));
    command.argc = 2;
    return update(key, command);
}

int CacheClient::chsize(const std::string& key, off_t new_size)
{
    UpdateCommand command;
    command.opcode = UpdateCode::to_byte(UpdateCode::Type::CHSIZE);
    command.argv.push_back(Utils::get_byte_array_from_int64(new_size));
    command.argc = 1;
    return update(key, command);
}

int CacheClient::rename(const std::string& old_key, const std::string& new_key)
{
    UpdateCommand command;
    command.opcode = UpdateCode::to_byte(UpdateCode::Type::RENAME);
    command.argv.push_back(Utils::get_byte_array_from_string(new_key));
    command.argc = 1;
    return update(old_key, command);
}