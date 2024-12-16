// #include "server_api.hpp"
#include "cache_api.hpp"
#include <iostream>

int main()
{
    try {
        ////// LOGGER //////
        spdlog::set_level(spdlog::level::trace); // Set global log level to debug
        spdlog::set_pattern("(%s:%#) [%^%l%$] %v");

        CacheServer object(8, "--SERVER=127.0.0.1");
        object.run(8888);
    } // try
    catch (std::exception& e){
        SPDLOG_ERROR("{}", e.what());
    }
    return 0;
}