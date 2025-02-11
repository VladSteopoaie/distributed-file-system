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

void Utils::struct_stat_to_proto(const struct stat* file_stat, Stat& proto_stat) {
    // std::cout << 1 << std::endl;
    // std::cout << file_stat << std::endl;
    // std::cout << file_stat->st_dev << std::endl;
    proto_stat.set_dev(file_stat->st_dev);
    // std::cout << proto_stat.dev() << std::endl;
    proto_stat.set_ino(file_stat->st_ino);
    proto_stat.set_mode(file_stat->st_mode);
    proto_stat.set_nlink(file_stat->st_nlink);
    proto_stat.set_uid(file_stat->st_uid);
    proto_stat.set_gid(file_stat->st_gid);
    proto_stat.set_rdev(file_stat->st_rdev);
    proto_stat.set_size(file_stat->st_size);
    proto_stat.set_blksize(file_stat->st_blksize);
    proto_stat.set_blocks(file_stat->st_blocks);
    proto_stat.set_atime(file_stat->st_atime);
    proto_stat.set_mtime(file_stat->st_mtime);
    proto_stat.set_ctime(file_stat->st_ctime);
}

void Utils::proto_to_struct_stat(const Stat& proto_stat, struct stat* file_stat) {
    file_stat->st_dev = proto_stat.dev();
    file_stat->st_ino = proto_stat.ino();
    file_stat->st_mode = proto_stat.mode();
    file_stat->st_nlink = proto_stat.nlink();
    file_stat->st_uid = proto_stat.uid();
    file_stat->st_gid = proto_stat.gid();
    file_stat->st_rdev = proto_stat.rdev();
    file_stat->st_size = proto_stat.size();
    file_stat->st_blksize = proto_stat.blksize();
    file_stat->st_blocks = proto_stat.blocks();
    file_stat->st_atime = proto_stat.atime();
    file_stat->st_mtime = proto_stat.mtime();
    file_stat->st_ctime = proto_stat.ctime();
}

std::vector<uint8_t> Utils::get_byte_array_from_int(uint32_t value)
{
    std::vector<uint8_t> byte_array = std::vector<uint8_t>(4);
    for (int i = 0; i < 4; i ++)
        byte_array[i] = static_cast<uint8_t>((value >> ((3 - i) * 8))) & 0xFF;
    return byte_array;
}

uint32_t Utils::get_int_from_byte_array(std::vector<uint8_t> byte_array)
{
    uint32_t value = 0;
    for (int i = 0; i < 4; i ++)
        value += static_cast<uint32_t> (byte_array[i] << (3 - i));
    return value;
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