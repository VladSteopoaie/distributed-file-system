#include "utils.hpp"

void Utils::read_conf_file(std::string conf_file, std::string& conf_string)
{
    try 
    {
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
    static std::random_device rd;
    static std::mt19937 gen(rd());

    static std::uniform_int_distribution<uint16_t> dist(1, std::numeric_limits<uint16_t>::max());

    return dist(gen);
}

void Utils::struct_stat_to_proto(const struct stat* object_stat, Stat& proto_stat) {
    proto_stat.set_dev(object_stat->st_dev);
    proto_stat.set_ino(object_stat->st_ino);
    proto_stat.set_mode(object_stat->st_mode);
    proto_stat.set_nlink(object_stat->st_nlink);
    proto_stat.set_uid(object_stat->st_uid);
    proto_stat.set_gid(object_stat->st_gid);
    proto_stat.set_rdev(object_stat->st_rdev);
    proto_stat.set_size(object_stat->st_size);
    proto_stat.set_blksize(object_stat->st_blksize);
    proto_stat.set_blocks(object_stat->st_blocks);
    proto_stat.set_atime(object_stat->st_atime);
    proto_stat.set_mtime(object_stat->st_mtime);
    proto_stat.set_ctime(object_stat->st_ctime);
}

void Utils::proto_to_struct_stat(const Stat& proto_stat, struct stat* object_stat) {
    object_stat->st_dev = proto_stat.dev();
    object_stat->st_ino = proto_stat.ino();
    object_stat->st_mode = proto_stat.mode();
    object_stat->st_nlink = proto_stat.nlink();
    object_stat->st_uid = proto_stat.uid();
    object_stat->st_gid = proto_stat.gid();
    object_stat->st_rdev = proto_stat.rdev();
    object_stat->st_size = proto_stat.size();
    object_stat->st_blksize = proto_stat.blksize();
    object_stat->st_blocks = proto_stat.blocks();
    object_stat->st_atime = proto_stat.atime();
    object_stat->st_mtime = proto_stat.mtime();
    object_stat->st_ctime = proto_stat.ctime();
}

std::vector<std::string> Utils::get_dir_list(const Stat& proto_stat)
{
    std::vector<std::string> result;

    for (const auto& entry : proto_stat.dir_list())
        result.push_back(entry);
    
    return result;
}

void Utils::set_dir_list(Stat& proto_stat, const std::vector<std::string>& dir_list)
{
    for (const std::string& path : dir_list)
        proto_stat.add_dir_list(path);
}

std::string Utils::process_path(std::string path, const std::string& absolute_path) {
    if (path.length() == 1 && path[0] == '.') {
        path = "/";
    }

    path = absolute_path + path;
    return path;
}

std::string Utils::get_parent_dir(std::string path) {
    if (path.empty() || path == "/")
        return "..";

    size_t last_slash = path.find_last_of('/');

    if (last_slash == std::string::npos || path.substr(0, last_slash).empty())
        return "/";

    return path.substr(0, last_slash);
}

void Utils::trim_trailing_nulls(std::vector<uint8_t>& vec) {
    auto it = std::find_if(vec.rbegin(), vec.rend(), [](uint8_t byte) {
        return byte != 0x00;
    });
    vec.erase(it.base(), vec.end());
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
        value += static_cast<uint32_t> (byte_array[i] << ((3 - i) * 8));
    return value;
}

std::vector<uint8_t> Utils::get_byte_array_from_int64(uint64_t value)
{
    std::vector<uint8_t> byte_array = std::vector<uint8_t>(8);
    for (int i = 0; i < 8; i ++)
        byte_array[i] = static_cast<uint8_t>((value >> ((7 - i) * 8))) & 0xFF;
    return byte_array;
}

uint64_t Utils::get_int64_from_byte_array(std::vector<uint8_t> byte_array)
{
    uint64_t value = 0;
    for (int i = 0; i < 8; i ++)
        value += static_cast<uint64_t> (byte_array[i] << ((7 - i) * 8));
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

// std::vector<Utils::ConnectionInfo> Utils::read_server_file(const std::string& server_file)
// {
//     std::vector<Utils::ConnectionInfo> connections;
//     std::ifstream file(server_file);
    
//     if (!file.is_open()) {
//         throw std::runtime_error("Could not open server file: " + server_file);
//     }

//     std::string line;
//     while (std::getline(file, line)) {
//         if (line.empty() || line[0] == '#') continue; // Skip empty lines and comments
//         size_t pos = line.find(':');
//         if (pos == std::string::npos) continue; // Invalid line format
//         std::string address = line.substr(0, pos);
//         uint16_t port = static_cast<uint16_t>(std::stoi(line.substr(pos + 1)));
//         connections.emplace_back(address, port);
//     }

//     file.close();
//     return connections;
// }