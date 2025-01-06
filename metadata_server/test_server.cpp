#include "lib/cache_server.hpp"
#include <iostream>

using namespace CacheAPI;

int main()
{
    try {
        ////// LOGGER //////
        spdlog::set_level(spdlog::level::trace); // Set global log level to debug
        spdlog::set_pattern("(%s:%#) [%^%l%$] %v");

        // CacheServer object(8, "--FILE=./memcached.conf", "./storage/");
        CacheServer object(8, "./storage/");
        object.run(8888);
    } // try
    catch (std::exception& e){
        SPDLOG_ERROR("{}", e.what());
    }
    return 0;
}