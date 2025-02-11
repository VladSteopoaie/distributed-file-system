#ifndef UTILS_HPP
#define UTILS_HPP

#include <libmemcached/memcached.h>

#include <spdlog.h>

#include <ctime>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <format>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>
#include <unistd.h>
#include "metadata.pb.h"

namespace Utils {
    void read_conf_file(std::string conf_file, std::string& conf_string);
    void prepare_conf_string(std::string& conf_string);
    uint16_t generate_id();
    void struct_stat_to_proto(const struct stat* file_stat, Stat& proto_stat);
    void proto_to_struct_stat(const Stat& proto_stat, struct stat* file_stat); 


    std::vector<uint8_t> get_byte_array_from_int(uint32_t value);
    uint32_t get_int_from_byte_array(std::vector<uint8_t> byte_array);
    std::vector<uint8_t> get_byte_array_from_string(std::string string); 
    std::string get_string_from_byte_array(std::vector<uint8_t> byte_array);
}

#endif