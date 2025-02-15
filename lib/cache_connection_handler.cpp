#include "cache_connection_handler.hpp"

using namespace CacheAPI;

// public
CacheConnectionHandler::CacheConnectionHandler(asio::io_context& context, memcached_st* mem_client, uint16_t mem_port, std::string storage_dir)
    : context(context)
    , socket(context)
    , mem_client(mem_client)
    , mem_port(mem_port)
    , storage_dir(storage_dir)
{}

CacheConnectionHandler::~CacheConnectionHandler() {
    socket.close();
}

tcp::socket& CacheConnectionHandler::get_socket() { return socket; }


void CacheConnectionHandler::start() 
{
    tcp::endpoint remote_endpoint =  socket.remote_endpoint();
    SPDLOG_INFO("New connection from {}:{}.",
        remote_endpoint.address().to_string(), remote_endpoint.port());
    read_socket_async();
}

// private
void CacheConnectionHandler::handle_error(std::string error)
{
    SPDLOG_ERROR(error);
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

    SPDLOG_DEBUG(std::format("Processing: {}", OperationCode::to_string(OperationCode::from_byte(request.opcode))));
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
        
        default:
            response.rescode = ResultCode::to_byte(ResultCode::Type::INVOP);
            break;
    }

}

void CacheConnectionHandler::set_memcached_object(std::string key, std::string value, time_t expiration, uint32_t flags)
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

void CacheConnectionHandler::set_memcached_object(std::string key, std::string value)
{
    try {
        set_memcached_object(key, value, 0, 0);
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(e.what());
    }
}

asio::awaitable<void> CacheConnectionHandler::set_memcached_object_async(std::string key, std::string value, time_t expiration, uint32_t flags)
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

std::string CacheConnectionHandler::get_memcached_object(std::string key)
{
    memcached_return_t error;
    char* result = memcached_get(mem_client, key.c_str(), key.length(), NULL, NULL, &error);

    if (result == NULL)
    {
        throw std::runtime_error(std::format("get_memcached_object: {}", memcached_strerror(mem_client, error)));
    }

    return result;
}


void CacheConnectionHandler::remove_memcached_object(std::string key) 
{
    memcached_return_t result;
    result = memcached_delete(mem_client, key.c_str(), key.length(), 0);

    if (result != MEMCACHED_SUCCESS)
    {
        throw std::runtime_error(std::format("set_memcached_object: {}", memcached_strerror(mem_client, result)));
    }
}

asio::awaitable<void> CacheConnectionHandler::remove_memcached_object_async(std::string key) 
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

std::string CacheConnectionHandler::set_local_file(std::string file_path, mode_t mode)
{
    // std::string file_path = storage_dir + path;
    SPDLOG_DEBUG(std::format("Path: {}", file_path));
    std::string result;
    Stat file_proto;
    struct stat file_stat;
    int fd;

    fd = open(file_path.c_str(), O_CREAT | O_RDWR, mode);
    if (fd < 0)
        throw std::runtime_error(std::format("set_local_file: {}", std::strerror(errno)));

    if (fstat(fd, &file_stat) != 0)
        throw std::runtime_error(std::format("set_local_file: {}", std::strerror(errno)));

    Utils::struct_stat_to_proto(&file_stat, file_proto);
    file_proto.SerializeToString(&result);
    if (write(fd, result.c_str(), result.length()) < 0)
        throw std::runtime_error(std::format("set_local_file: {}", std::strerror(errno)));

    return result;
}

std::string CacheConnectionHandler::set_local_dir(std::string dir_path, mode_t mode)
{
    // std::string dir_path = storage_dir + path;
    SPDLOG_DEBUG(std::format("Path: {}", dir_path));
    std::string result_string;
    Stat dir_proto;
    struct stat dir_stat;
    int result;

    result = mkdir(dir_path.c_str(), mode);
    if (result != 0)
        throw std::runtime_error(std::format("set_local_dir: {}", std::strerror(errno)));

    if (stat(dir_path.c_str(), &dir_stat) != 0)
        throw std::runtime_error(std::format("set_local_dir: {}", std::strerror(errno)));

    Utils::struct_stat_to_proto(&dir_stat, dir_proto);
    dir_proto.SerializeToString(&result_string);


    return result_string;
}

void CacheConnectionHandler::set(const CachePacket& request, CachePacket& response, bool is_file)
{
    try {
        uint32_t flags = request.flags;
        time_t time = static_cast<time_t>(request.time);
        std::string path = Utils::get_string_from_byte_array(request.key);
        Utils::process_path(path, storage_dir);
        //path = storage_dir + path;
        mode_t mode = std::stoul(Utils::get_string_from_byte_array(request.value));

        std::string value;
        if (is_file)
            value = set_local_file(path, mode);
        else
            value = set_local_dir(path, mode);
        
        asio::co_spawn(context, set_memcached_object_async(path, value, time, flags), asio::detached);
        
        // when creating an object we need to update the parent directory in 
        // the memcached server
        std::string dir_path = Utils::get_parent_dir(path);
        std::string dir_value = get_local_dir(dir_path);
        asio::co_spawn(context, set_memcached_object_async(dir_path, dir_value, time, flags), asio::detached);

        response.rescode = ResultCode::to_byte(ResultCode::Type::SUCCESS);
    }
    catch (std::exception& e)
    {
        std::string message = std::format("set: {}", e.what());
        SPDLOG_ERROR(message);
        response.rescode = ResultCode::to_byte(ResultCode::Type::ERRMSG);
        response.message_len = 4;
        response.message = Utils::get_byte_array_from_int(errno);
    }
}

std::string CacheConnectionHandler::get_local_file(std::string file_path)
{
    // std::string file_path = storage_dir + key;
    SPDLOG_DEBUG(std::format("Path: {}", file_path));
    std::ifstream file(file_path);

    if (!file)
        throw std::runtime_error(std::format("get_local_file: {}", std::strerror(errno)));

    std::ostringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

std::string CacheConnectionHandler::get_local_dir(std::string dir_path)
{
    Stat dir_proto;
    struct stat dir_stat;
    struct dirent* entry;
    // std::string dir_path = storage_dir + key;
    SPDLOG_DEBUG(std::format("Path: {}", dir_path));
    DIR* dir = opendir(dir_path.c_str());
    std::string result;

    if (!dir)
        throw std::runtime_error(std::format("get_local_dir: {}", std::strerror(errno)));

    if (stat(dir_path.c_str(), &dir_stat) != 0)
        throw std::runtime_error(std::format("get_local_dir: {}", std::strerror(errno)));

    Utils::struct_stat_to_proto(&dir_stat, dir_proto);

    errno = 0;
    while ((entry = readdir(dir)) != nullptr)
        dir_proto.add_dir_list(entry->d_name);

    if (errno != 0)
        throw std::runtime_error(std::format("get_local_dir: {}", std::strerror(errno)));


    dir_proto.SerializeToString(&result);

    return result;
}

void CacheConnectionHandler::get(const CachePacket& request, CachePacket& response, bool is_file)
{
    try {
        std::string path = Utils::get_string_from_byte_array(request.key);
        Utils::process_path(path, storage_dir);
        //path = storage_dir + path;
        std::string value;

        if (is_file)
            value = get_local_file(path);
        else
            value = get_local_dir(path);

        if (!value.empty())
            asio::co_spawn(context, set_memcached_object_async(path, value, 0, 0), asio::detached);

        response.rescode = ResultCode::to_byte(ResultCode::Type::SUCCESS);
        response.value_len = value.length();
        response.value = Utils::get_byte_array_from_string(value);
    }
    catch (std::exception& e)
    {
        std::string message = std::format("get: {}", e.what());
        SPDLOG_ERROR(message);
        response.rescode = ResultCode::to_byte(ResultCode::Type::ERRMSG);
        response.message_len = 4;
        response.message = Utils::get_byte_array_from_int(errno);
        // response.message_len = static_cast<uint16_t>(message.length());
        // response.message = Utils::get_byte_array_from_string(message);
    }

}


void CacheConnectionHandler::remove_local_file(std::string path) 
{
    int res = unlink(path.c_str());
    if (res != 0)
        throw std::runtime_error(std::format("remove_local_dir: {}", std::strerror(errno)));
}

void CacheConnectionHandler::remove_local_dir(std::string path) 
{
    int res = rmdir(path.c_str());
    if (res != 0)
        throw std::runtime_error(std::format("remove_local_dir: {}", std::strerror(errno)));
}

void CacheConnectionHandler::remove(const CachePacket& request, CachePacket& response, bool is_file) 
{
    try {
        std::string path = Utils::get_string_from_byte_array(request.key);
        Utils::process_path(path, storage_dir);

        if (is_file)
            remove_local_file(path);
        else
            remove_local_dir(path);

        asio::co_spawn(context, remove_memcached_object_async(path), asio::detached);

        response.rescode = ResultCode::to_byte(ResultCode::Type::SUCCESS);
    }
    catch (std::exception& e)
    {
        std::string message = std::format("get: {}", e.what());
        SPDLOG_ERROR(message);
        response.rescode = ResultCode::to_byte(ResultCode::Type::ERRMSG);
        response.message_len = 4;
        response.message = Utils::get_byte_array_from_int(errno);
        // response.message_len = static_cast<uint16_t>(message.length());
        // response.message = Utils::get_byte_array_from_string(message);
    }

}
