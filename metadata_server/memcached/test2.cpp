#include "cache_api.hpp"
#include <iostream>

int main()
{
    try {
        ////// LOGGER //////
        spdlog::set_level(spdlog::level::trace); // Set global log level to debug
        spdlog::set_pattern("(%s:%#) [%^%l%$] %v");

        ///// ASIO /////
        asio::io_context context;

        // CacheInterface object({"127.0.0.1", 1337}, "--FILE=./memcached.conf");
        CacheServer object(context, "127.0.0.1", 1337, "--FILE=./memcached.conf");
    
        // object.cache_init();
        object.cache_set_object("bar", "hai hui");
            
        std::string value = object.cache_get_object("bar");
        std::cout << value << std::endl;
    } // try
    catch (std::exception& e){
        // spdlog::error(e.what());
        SPDLOG_ERROR("{}", e.what());
    }
    return 0;
}