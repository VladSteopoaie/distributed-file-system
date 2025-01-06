#include "lib/cache_client.hpp"
#include <iostream>
#include <unistd.h>

// Alias for readability
using asio::ip::tcp;
using namespace CacheAPI;

int main() {
    try {
        ////// LOGGER //////
        // spdlog::set_level(spdlog::level::trace); // Set global log level to debug
        spdlog::set_pattern("(%s:%#) [%^%l%$] %v");

        CacheClient client;

        client.connect("127.0.0.1", "8888");
        // client.set("foo", "bar");
        std::string val = client.get("foo");
        std::cout << val << std::endl;
        client.set("blabla", "ce faci");
        std::cout << client.get("blabla") << std::endl;
        std::cout << "Nope: " << client.get("aaaa") << std::endl;
    } 
    catch (std::exception& e) {
        SPDLOG_ERROR("Exception: {}", e.what());
    }
    return 0;
}
