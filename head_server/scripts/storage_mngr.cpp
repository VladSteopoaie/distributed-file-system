#include "../lib/storage_server.hpp"
// #include "../../lib/storage_server.hpp"

#include <iostream>
#include <CLI11.hpp>

using namespace StorageAPI;

int main(int argc, char** argv)
{
    CLI::App app {"Storage manager."};
    argv = app.ensure_utf8(argv);

    uint8_t thread_count = 8;
    uint16_t port = 7777;
    int stripe_size = 4096;
    std::string server_file = "storage.conf";

    app.add_option("-p, --port", port, "Port on which to run the server.")->check(CLI::Range(1, 65535));
    app.add_option("-t, --threads", thread_count, "Number of threads in the thread pool.")->check(CLI::Range(1, 16))->required();
    app.add_option("-s, --stripe-size", stripe_size, "Stripe size to break down large files.")->check(CLI::Range(1, 131072));
    app.add_option("-f, --file", server_file, "Configuration file for the storage servers.")->check(CLI::ExistingFile)->required();

    CLI11_PARSE(app, argc, argv);

    std::vector<Utils::ConnectionInfo<StoragePacket>> connections = Utils::ConnectionInfo<StoragePacket>::read_server_file(server_file);

    try {
        ////// LOGGER //////
        spdlog::set_level(spdlog::level::debug); // Set global log level to debug
        spdlog::set_pattern("(%s:%#) [%^%l%$] %v");
        
        StorageServer object((int)thread_count, stripe_size, connections);
        object.run(port);
    }
    catch (std::exception& e){
        SPDLOG_ERROR("{}", e.what());
    }
    return 0;
}