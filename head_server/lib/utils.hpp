#ifndef UTILS_HPP
#define UTILS_HPP

#include <libmemcached/memcached.h>

#include <spdlog.h>

#include <ctime>
#include <sstream>
#include <string>
#include <cstring>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <cstdint>
#include <format>
#include <random>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>

namespace Utils {
    void read_conf_file(std::string conf_file, std::string& conf_string);
    void prepare_conf_string(std::string& conf_string);
    uint16_t generate_id();
    std::vector<uint8_t> get_byte_array_from_string(std::string string); 
    std::string get_string_from_byte_array(std::vector<uint8_t> byte_array);
}

#endif