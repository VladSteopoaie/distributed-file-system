#include "cache_server.hpp"

using namespace CacheAPI;

template class GenericServer<CacheConnectionHandler>;

void CacheServer::init() {
    // This method can be used to perform any additional initialization if needed.
    // Currently, it is not used, but can be extended in the future.
}

CacheServer::CacheServer(int thread_count, std::string mem_conf_string, std::string file_metadata_dir, std::string dir_metadata_dir)
    : GenericServer<CacheConnectionHandler>::GenericServer(thread_count)
    , mem_conf_string(mem_conf_string)
    , memcached_pid(-1)
    , mem_port(0)
    , file_metadata_dir(file_metadata_dir)
    , dir_metadata_dir(dir_metadata_dir)
{
    try {
        if (!std::filesystem::exists(file_metadata_dir))
        {
            throw std::runtime_error(std::format("No directory {} found.", file_metadata_dir));
        }
        else if (!std::filesystem::is_directory(file_metadata_dir))
        {
            throw std::runtime_error(std::format("{} it's not a directory.", file_metadata_dir));   
        } 

        if (file_metadata_dir[file_metadata_dir.length() - 1] != '/')
            this->file_metadata_dir = file_metadata_dir + "/";

        ////// MEMCACHED CONNECTION //////
        if (mem_conf_string.length() == 0)
        {
            throw std::runtime_error("Configuration string is empty!");
        }

        Utils::prepare_conf_string(mem_conf_string);

        // initializing connectivity
        mem_client = memcached(mem_conf_string.c_str(), mem_conf_string.length());

        if (mem_client == NULL)
        {
            throw std::runtime_error("Memcached error when initializing connectivity!");
        }
    } // try
    catch (std::exception& e)
    {
        throw std::runtime_error(std::format("CacheServer: {}", e.what()));
    }
}

CacheServer::CacheServer(int thread_count, uint16_t mem_port,std::string file_metadata_dir, std::string dir_metadata_dir)
    : GenericServer<CacheConnectionHandler>::GenericServer(thread_count)
    , mem_port(mem_port)
    , mem_conf_string(std::format("--SERVER=127.0.0.1:{}", mem_port))
    , file_metadata_dir(file_metadata_dir)
    , dir_metadata_dir(dir_metadata_dir)
{
    if (!std::filesystem::exists(file_metadata_dir) || !std::filesystem::exists(dir_metadata_dir))
    {
        throw std::runtime_error(std::format("No directory {} or {} found.", file_metadata_dir, dir_metadata_dir));
    }
    else if (!std::filesystem::is_directory(file_metadata_dir))
    {
        throw std::runtime_error(std::format("{} or {} it's not a directory.", file_metadata_dir, dir_metadata_dir));   
    }

    if (file_metadata_dir[file_metadata_dir.length() - 1] != '/')
        this->file_metadata_dir += "/";
    if (dir_metadata_dir[dir_metadata_dir.length() - 1] != '/')
        this->dir_metadata_dir += "/";

    // starting a memcached server
    SPDLOG_INFO("CacheServer: Starting memcached server on 0.0.0.0:{}.", mem_port);
    memcached_pid = fork();

    if (memcached_pid == 0) // child process
    {
        // preparing the command
        char* const args[] = {(char*)"memcached", (char*)"-p", (char*)std::to_string(mem_port).c_str(), nullptr};
        int result = execvp("memcached", args);

        if (result != 0)
            SPDLOG_ERROR("execvp: {}", strerror(errno));

        exit(0);
    }
    else if (memcached_pid > 0) // parent process
    {
        // check validity of conf_string
        Utils::prepare_conf_string(mem_conf_string);

        // initializing connectivity
        mem_client = memcached(mem_conf_string.c_str(), mem_conf_string.length());

        if (mem_client == NULL)
        {
            throw std::runtime_error("CacheServer: Memcached error when initializing connectivity!");
        }
    }
    else 
    {
        throw std::runtime_error(std::format("fork: {}", strerror(errno)));
    }
}

CacheServer::CacheServer(int thread_count, std::string file_metadata_dir, std::string dir_metadata_dir)
    : CacheServer(thread_count, 11211, file_metadata_dir, dir_metadata_dir) // default memcached port -> 11211
{}

CacheServer::CacheServer(std::string file_metadata_dir, std::string dir_metadata_dir)
    : CacheServer(1, file_metadata_dir, dir_metadata_dir) // default mode -> 1 thread
{}

CacheServer::~CacheServer()
{
    if (memcached_pid > 0) {
        SPDLOG_INFO("Terminating memcached server.");
        kill(memcached_pid, SIGKILL);
    }

    if (mem_client != NULL)
        memcached_free(mem_client);
    SPDLOG_INFO("Exiting Cache Server.");
}

void CacheServer::run(uint16_t port) {
    GenericServer<CacheConnectionHandler>::run(port, mem_client, mem_port, file_metadata_dir, dir_metadata_dir);
}
