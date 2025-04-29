#include "cache_connection_handler.hpp"

using namespace CacheAPI;

// public
CacheConnectionHandler::CacheConnectionHandler(asio::io_context& context, memcached_st* mem_client, uint16_t mem_port, std::string file_metadata_dir, std::string dir_metadata_dir)
    : GenericConnectionHandler<CachePacket>::GenericConnectionHandler(context)
    , mem_client(mem_client)
    , mem_port(mem_port)
    , file_metadata_dir(file_metadata_dir)
    , dir_metadata_dir(dir_metadata_dir) {}

// private
void CacheConnectionHandler::handle_request(const CachePacket& request, CachePacket& response)
{
    if (request.id == 0)
    {
        response.rescode = ResultCode::to_byte(ResultCode::Type::INVPKT);
        return;
    }

    response.id = request.id; 
    response.opcode = request.opcode;

    SPDLOG_DEBUG(std::format("Processing: {}", OperationCode::to_string(OperationCode::from_byte(request.opcode))));
    try {
        switch (OperationCode::from_byte(request.opcode))
        {
            case OperationCode::Type::NOP:
                response.rescode = ResultCode::to_byte(ResultCode::Type::SUCCESS);
                break;
            case OperationCode::Type::INIT:
                init_connection(response);
                break;
            case OperationCode::Type::GET_FILE:
                get(request, response, true);
                break;
            case OperationCode::Type::GET_DIR:
                get(request, response, false);
                break;
            case OperationCode::Type::SET_FILE:
                set(request, response, true);
                break;
            case OperationCode::Type::SET_DIR:
                set(request, response, false);
                break;
            case OperationCode::Type::RM_FILE:
                remove(request, response, true);
                break;
            case OperationCode::Type::RM_DIR:
                remove(request, response, false);
                break;
            case OperationCode::Type::UPDATE:
                update(request, response);
                break;
            
            default:
                response.rescode = ResultCode::to_byte(ResultCode::Type::INVOP);
                break;
        } // switch
    }
    catch (std::exception& e)
    {
        SPDLOG_ERROR(e.what());
        response.rescode = ResultCode::to_byte(ResultCode::Type::ERRMSG);
        response.message_len = 4;
        response.message = Utils::get_byte_array_from_int(errno);
    }
} // handle_request

void CacheConnectionHandler::update_parent_dir(const std::string& path)
{
    try 
    {
        std::string parent_path = Utils::get_parent_dir(path);
        std::string parent_value = FileMngr::get_local_dir(file_metadata_dir + parent_path, dir_metadata_dir + parent_path, true);
        asio::co_spawn(context, set_memcached_object_async(parent_path, parent_value, 0, 0), asio::detached);
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(std::format("update_parent_dir: {}", e.what()));
    }
}

void CacheConnectionHandler::update_memcached_object(const std::string& key, const std::string& value, time_t expiration, uint32_t flags)
{
    memcached_return_t result = memcached_replace(mem_client,
    key.c_str(), key.length(),
    value.c_str(), value.length(),
    expiration, flags);

    if (result != MEMCACHED_SUCCESS)
    {
        throw std::runtime_error(std::format("update_memcached_object: {}", memcached_strerror(mem_client, result)));
    }
}

asio::awaitable<void> CacheConnectionHandler::update_memcached_object_async(const std::string& key, const std::string& value, time_t expiration, uint32_t flags)
{
    try {
        update_memcached_object(key, value, expiration, flags);
    }
    catch (std::exception& e)
    {
        SPDLOG_ERROR(std::format("update_memcached_object_async: {}", e.what()));
    }

    co_return;
}

void CacheConnectionHandler::set_memcached_object(const std::string& key, const std::string& value, time_t expiration, uint32_t flags)
{
    memcached_return_t result = memcached_set(mem_client, 
            key.c_str(), key.size(), 
            value.c_str(), value.size(),
            expiration, flags);

    if (result != MEMCACHED_SUCCESS)
    {
        throw std::runtime_error(std::format("set_memcached_object: {}", memcached_strerror(mem_client, result)));
    }
}

void CacheConnectionHandler::set_memcached_object(const std::string& key, const std::string& value)
{
    try {
        set_memcached_object(key, value, 0, 0);
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(e.what());
    }
}

asio::awaitable<void> CacheConnectionHandler::set_memcached_object_async(const std::string& key, const std::string& value, time_t expiration, uint32_t flags)
{
    try {
        set_memcached_object(key, value, expiration, flags);
    }
    catch (std::exception& e)
    {
        SPDLOG_ERROR(std::format("set_memcached_object_async: {}", e.what()));
    }

    co_return;
}

std::string CacheConnectionHandler::get_memcached_object(const std::string& key)
{
    memcached_return_t error;
    char* result = memcached_get(mem_client, key.c_str(), key.length(), NULL, NULL, &error);

    if (result == NULL)
    {
        throw std::runtime_error(std::format("get_memcached_object: {}", memcached_strerror(mem_client, error)));
    }

    return result;
}

void CacheConnectionHandler::remove_memcached_object(const std::string& key) 
{
    memcached_return_t result;
    result = memcached_delete(mem_client, key.c_str(), key.length(), 0);

    if (result != MEMCACHED_SUCCESS)
    {
        throw std::runtime_error(std::format("set_memcached_object: {}", memcached_strerror(mem_client, result)));
    }
}

asio::awaitable<void> CacheConnectionHandler::remove_memcached_object_async(const std::string& key) 
{
    try {
        remove_memcached_object(key);
    }
    catch (std::exception& e)
    {
        SPDLOG_ERROR(std::format("remove_memcached_object_async: {}", e.what()));
    }

    co_return;
}

void CacheConnectionHandler::init_connection(CachePacket& response)
{
    // sending the memcached port if it's set
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

void CacheConnectionHandler::set(const CachePacket& request, CachePacket& response, bool is_file)
{
    try {
        uint32_t flags = request.flags;
        time_t time = static_cast<time_t>(request.time);
        std::string path = Utils::get_string_from_byte_array(request.key);
        std::string file_path = Utils::process_path(path, file_metadata_dir);
        std::string dir_path = Utils::process_path(path, dir_metadata_dir);
        
        mode_t mode = std::stoul(Utils::get_string_from_byte_array(request.value));

        std::string value;
        if (is_file)
        {
            value = FileMngr::set_local_file(file_path, mode);
        }
        else
        {
            // creating two directories
            // 1. for the folder hierarchy of the file system
            // FileMngr::set_local_dir(file_path, mode);
            // 2. for storing the metadata about the directory (the file with metadata will be stored in .this file)
            // inside the directory
            FileMngr::set_local_dir(file_path, dir_path, mode);
            value = FileMngr::get_local_dir(file_path, dir_path, true);
        }

        asio::co_spawn(context, set_memcached_object_async(path, value, time, flags), asio::detached);
        
        // when creating an object we need to update the parent directory in 
        // the memcached server
        update_parent_dir(path);
        response.rescode = ResultCode::to_byte(ResultCode::Type::SUCCESS);
    } // try
    catch (std::exception& e)
    {
        throw std::runtime_error(std::format("set: {}", e.what()));
    }
} // set

void CacheConnectionHandler::get(const CachePacket& request, CachePacket& response, bool is_file)
{
    try {
        std::string path = Utils::get_string_from_byte_array(request.key);
        std::string file_path = Utils::process_path(path, file_metadata_dir);
        std::string dir_path = Utils::process_path(path, dir_metadata_dir);
        std::string value;

        if (is_file)
            value = FileMngr::get_local_file(file_path);
        else
            value = FileMngr::get_local_dir(file_path, dir_path);

        if (!value.empty())
            asio::co_spawn(context, set_memcached_object_async(path, value, 0, 0), asio::detached);

        response.rescode = ResultCode::to_byte(ResultCode::Type::SUCCESS);
        response.value_len = value.length();
        response.value = Utils::get_byte_array_from_string(value);
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(std::format("get: {}", e.what()));
    }

} // get

void CacheConnectionHandler::remove(const CachePacket& request, CachePacket& response, bool is_file)
{
    try {
        std::string path = Utils::get_string_from_byte_array(request.key);
        std::string file_path = Utils::process_path(path, file_metadata_dir);
        std::string dir_path = Utils::process_path(path, dir_metadata_dir);

        if (is_file)
            FileMngr::remove_local_file(file_path);
        else
            FileMngr::remove_local_dir(file_path, dir_path);

        asio::co_spawn(context, remove_memcached_object_async(path), asio::detached);
        update_parent_dir(path);    
        response.rescode = ResultCode::to_byte(ResultCode::Type::SUCCESS);
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(std::format("remove: {}", e.what()));
    }
} // remove

void CacheConnectionHandler::update(const CachePacket& request, CachePacket& response) 
{
    try {
        bool is_file;
        std::string path = Utils::get_string_from_byte_array(request.key);
        std::string file_path = Utils::process_path(path, file_metadata_dir);
        std::string dir_path = Utils::process_path(path, dir_metadata_dir);
        std::string value, rename_content;
        UpdateCommand command = UpdateCommand(request.value.data(), request.value.size());

        if (std::filesystem::is_regular_file(file_path))
            is_file = true;
        else if (std::filesystem::is_directory(file_path))
            is_file = false;
        else
        {
            errno = ENOENT;
            throw std::runtime_error(std::strerror(errno));
        }

        if (UpdateCode::from_byte(command.opcode) == UpdateCode::RENAME)
        {
            if (is_file)
                rename_content = FileMngr::get_local_file(file_path);
            else
                rename_content = FileMngr::get_local_dir(file_path, dir_path);
        }

        if (is_file)
            value = FileMngr::update_local_file(path, file_metadata_dir, command);
        else
            value = FileMngr::update_local_dir(path, file_metadata_dir, dir_metadata_dir, command);

        if (UpdateCode::from_byte(command.opcode) == UpdateCode::RENAME)
        {
            asio::co_spawn(context, remove_memcached_object_async(path), asio::detached);
            asio::co_spawn(context, set_memcached_object_async(value, rename_content, 0, 0), asio::detached);
            update_parent_dir(path);
        }
        else
        {
            asio::co_spawn(context, update_memcached_object_async(path, value, 0, 0), asio::detached);
        }

        response.rescode = ResultCode::to_byte(ResultCode::Type::SUCCESS);
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(std::format("update: {}", e.what()));
    }
} // update
