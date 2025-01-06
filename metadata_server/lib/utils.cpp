#include "utils.hpp"

void Utils::read_conf_file(std::string conf_file, std::string& conf_string)
{
    try {
        std::ifstream file (conf_file);

        if (!file)
        {
            throw std::runtime_error(strerror(errno));
        }

        std::string content;
        conf_string.clear();
        
        file >> content;
        conf_string += content;

        while (file >> content)
            conf_string += " " + content;

        file.close();
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(std::format("read_conf_file: {}", e.what()));
    }
}

void Utils::prepare_conf_string(std::string& mem_conf_string)
{
    try {
        // search if a file was given
        size_t start_pos = mem_conf_string.find("--FILE=");
            
        if (start_pos != std::string::npos)
        {
            size_t end_pos = start_pos + std::string("--FILE=").length();
            std::string mem_conf_file = mem_conf_string.substr(end_pos);
            Utils::read_conf_file(mem_conf_file, mem_conf_string);
        }

        // check validity of conf_string
        char conf_error[500];

        if (libmemcached_check_configuration(mem_conf_string.c_str(), mem_conf_string.length(), conf_error, sizeof(conf_error)) != MEMCACHED_SUCCESS)
        {   
            throw std::runtime_error(std::format("Invalid configuration string! [{}]", conf_error));
        }
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(std::format("prepare_conf_string: {}", e.what()));
    }
}

uint16_t Utils::generate_id()
{
     // Create a random device and seed the random number generator
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // Define the range for uint16_t, excluding 0
    static std::uniform_int_distribution<uint16_t> dist(1, std::numeric_limits<uint16_t>::max());

    // Generate and return a random value
    return dist(gen);
}

// From: https://github.com/VladSteopoaie/DNS-tunneling/blob/main/dns_server/modules/dns_module.cpp

// Conversion funtions
std::vector<uint8_t> Utils::get_byte_array_from_string(std::string string)
{
    std::vector<uint8_t> byte_array = std::vector<uint8_t>(string.size());
    for (int i = 0; i < string.size(); i ++)
        byte_array[i] = string[i];
    return byte_array;
}

std::string Utils::get_string_from_byte_array(std::vector<uint8_t> byte_array)
{
    std::string result(byte_array.size(), '\0');

    for (int i = 0; i < byte_array.size(); i ++){
        result[i] = byte_array[i];
    }
    return result;
}