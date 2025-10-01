#include "../lib/storage_server.hpp"
// #include "../../lib/storage_server.hpp"
#include <iostream>
#include <CLI11.hpp>
// #include <mpi.h>

using namespace StorageAPI;

int main(int argc, char** argv)
{
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        std::cerr << "MPI does not support multiple threads!" << std::endl;
        exit(1);
    }

    CLI::App app {"Storage manager."};
    argv = app.ensure_utf8(argv);

    uint8_t thread_count = 8;
    uint16_t port = 7777;
    int stripe_size = 4096;

    app.add_option("-p, --port", port, "Port on which to run the server.")->check(CLI::Range(1, 65535));
    app.add_option("-t, --threads", thread_count, "Number of threads in the thread pool.")->check(CLI::Range(1, 16))->required();
    app.add_option("-s, --stripe-size", stripe_size, "Stripe size to break down large files.")->check(CLI::Range(1, 131072));
    CLI11_PARSE(app, argc, argv);

    try {
        ////// LOGGER //////
        spdlog::set_level(spdlog::level::debug); // Set global log level to debug
        spdlog::set_pattern("(%s:%#) [%^%l%$] %v");

        // CacheServer object(8, "--FILE=./memcached.conf", "./storage/");
        StorageServer object((int)thread_count, stripe_size);
        object.run(port);
    }
    catch (std::exception& e){
        SPDLOG_ERROR("{}", e.what());
    }

    MPI_Finalize();
    return 0;
}