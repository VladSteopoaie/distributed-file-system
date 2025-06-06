#ifndef UTILS_HPP
#define UTILS_HPP

#include <libmemcached/memcached.h>

#include <spdlog.h>

#include <ctime>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <chrono>
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

namespace Utils 
{
    void read_conf_file(std::string conf_file, std::string& conf_string);
    void prepare_conf_string(std::string& conf_string);
    uint16_t generate_id();
    void struct_stat_to_proto(const struct stat* object_stat, Stat& proto_stat);
    void proto_to_struct_stat(const Stat& proto_stat, struct stat* object_stat); 
    std::vector<std::string> get_dir_list(const Stat& proto_stat);
    void set_dir_list(Stat& proto_stat, const std::vector<std::string>& dir_list);
    std::string process_path(std::string path, const std::string& absolute_path);
    std::string get_parent_dir(std::string path);

    void trim_trailing_nulls(std::vector<uint8_t>& vec);
    std::vector<uint8_t> get_byte_array_from_int(uint32_t value);
    uint32_t get_int_from_byte_array(std::vector<uint8_t> byte_array);
    std::vector<uint8_t> get_byte_array_from_int64(uint64_t value);
    uint64_t get_int64_from_byte_array(std::vector<uint8_t> byte_array);
    std::vector<uint8_t> get_byte_array_from_string(std::string string); 
    std::string get_string_from_byte_array(std::vector<uint8_t> byte_array);

    class PerformanceTimer {
    private:
        // std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
        // std::chrono::time_point<std::chrono::high_resolution_clock> end_time;
        std::string operation_name;
        std::ofstream& log_file;

    public:
        PerformanceTimer(const std::string& name, std::ofstream& file) 
            : operation_name(name), log_file(file) {
            // log_file.open(file, std::ios::app);
            // start_time = std::chrono::high_resolution_clock::now();
            log_file << operation_name << " start: " << std::chrono::high_resolution_clock::now() << " microseconds\n";
        }
        
        ~PerformanceTimer() {
            // end_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).count();
            // end_time = std::chrono::high_resolution_clock::now();
            // auto endTime = std::chrono::high_resolution_clock::now();
            // auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - start_time).count();
            log_file << operation_name << " end: " << std::chrono::high_resolution_clock::now() << " microseconds\n";
            log_file.flush();
        }
    };
}

#endif