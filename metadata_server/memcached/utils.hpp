#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <cstring>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <cstdint>
#include <format>


namespace Utils {
    void read_conf_file(std::string conf_file, std::string& conf_string);
    std::vector<uint8_t> get_byte_array_from_string(std::string string); 
    std::string get_string_from_byte_array(std::vector<uint8_t> byte_array);
}

#endif