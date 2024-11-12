#include "cache_api.h"
#include <cstring>
#include <libmemcached/memcached.h>
#include <vector>

// compile: gcc test.cpp -o test -lmemcached

int main(int argc, char** argv)
{
    std::string conf_string(CONF_MAX_SIZE), conf_error(CONF_MAX_SIZE);

    if (read_conf_file(argv[1], conf_string) < 0)
    {
        return -1;
    }

    if (libmemcached_check_configuration(conf_string, strlen(conf_string), conf_error, sizeof(conf_error)) != MEMCACHED_SUCCESS)
    {
        std::cerr << conf_error << std::endl;
        return -1;
    }


    memcached_st *mem_client = memcached(conf_string, strlen(conf_string));
    
    // setting values in the cache
    for (int i = 1; i <= 10; i++)
    {
        memcached_return_t result;
        std::string key = "foo" + std::to_string(i);
        std::string value = "bar" + std::to_string(i);
        result = memcached_set(
            mem_client, 
            key.c_str(),
            key.size(),
            value.c_str(),
            value.size(),
            0,
            0);

        if (result != MEMCACHED_SUCCESS)
        {
            std::cerr << "Failed: " << memcached_strerror(mem_client, result) << std::endl;
            return -1;
        }

    }

    memcached_free(mem_client);
    return 0;
}