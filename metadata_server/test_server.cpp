#include "./lib/cache_server.hpp"
#include <iostream>
#include <CLI11.hpp>

using namespace CacheAPI;

int main(int argc, char** argv)
{
    CLI::App app {"Cache server."};
    argv = app.ensure_utf8(argv);

    std::string file_meta, dir_meta;
    uint8_t thread_count = 1;
    uint16_t port = 8888;
    uint16_t mem_port = 11211;

    app.add_option("-f, --file-meta", file_meta, "A directory to store cached file metadata.")->required();
    app.add_option("-d, --dir-meta", dir_meta, "A directory to store cached directory metadata.")->required();
    app.add_option("-p, --port", port, "Port on which to run the server.")->check(CLI::Range(1, 65535));
    app.add_option("-m, --mport", mem_port, "Port on which to run the MEMECACHED server.")->check(CLI::Range(1, 65535));
    app.add_option("-t, --threads", thread_count, "Number of threads in the thread pool.")->check(CLI::Range(1, 16))->required();
    CLI11_PARSE(app, argc, argv);

    try {
        ////// LOGGER //////
        spdlog::set_level(spdlog::level::debug); // Set global log level to debug
        spdlog::set_pattern("(%s:%#) [%^%l%$] %v");

        // CacheServer object(8, "--FILE=./memcached.conf", "./storage/");
        CacheServer object((int)thread_count, mem_port, file_meta, dir_meta);
        object.run(port);
    } 
    catch (std::exception& e){
        SPDLOG_ERROR("{}", e.what());
    }
    return 0;
}