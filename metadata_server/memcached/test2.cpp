#include "cache_api.h"
#include <iostream>

int main()
{
    ////// LOGGER //////
    spdlog::set_level(spdlog::level::trace); // Set global log level to debug
    spdlog::set_pattern("(%s:%#) [%^%l%$] %v");


    // CacheInterface object({"127.0.0.1", 1337}, "--FILE=./memcached.conf");
    CacheServer object({"127.0.0.1", 1337}, "--FILE=./memcached.conf");
    if (object.cache_init() != 0)
        return -1;
    
    if (object.cache_set_object("bar", "hai hui") != 0)
        return -1;
        
    std::string value = object.cache_get_object("bar");
    std::cout << value << std::endl;
    return 0;
}