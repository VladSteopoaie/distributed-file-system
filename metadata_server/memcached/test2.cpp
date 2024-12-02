// #include "server_api.hpp"
#include "cache_api.hpp"
#include <iostream>

class ConnH {
private:
    asio::io_context& context;
    tcp::socket socket;

public:
    ConnH(asio::io_context& context) : context(context), socket(context) {}

    tcp::socket& get_socket() { return socket; }

    void start() { std::cout << "Hello world!" << std::endl; }
};

int main()
{
    try {
        ////// LOGGER //////
        spdlog::set_level(spdlog::level::trace); // Set global log level to debug
        spdlog::set_pattern("(%s:%#) [%^%l%$] %v");

        ///// ASIO /////
        // asio::io_context context;

        // CacheInterface object({"127.0.0.1", 1337}, "--FILE=./memcached.conf");
        // CacheServer object(context, "127.0.0.1", 1337, "--FILE=./memcached.conf");
        CacheServer object(1, "--FILE=./memcached.conf");
        // GenericServer<ConnH> object(1);
        // object.cache_init();
        // object.cache_set_object("bar", "hai hui");
            
        // std::string value = object.cache_get_object("bar");
        // std::cout << value << std::endl;

        object.run(8888);
    } // try
    catch (std::exception& e){
        // spdlog::error(e.what());
        SPDLOG_ERROR("{}", e.what());
    }
    return 0;
}