#ifndef UTILS_HPP
#define UTILS_HPP

#include <libmemcached/memcached.h>

// #include <spdlog.h>

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

#ifndef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif
#include <spdlog/spdlog.h>

#define ASIO_STANALONE // non-boost version
#define ASIO_NO_DEPRECATED // no need for deprecated stuff
#define ASIO_HAS_STD_COROUTINE // c++20 coroutines needed
#include <asio.hpp>

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

    template<typename Packet>
    struct ConnectionInfo {
        std::string address;
        uint16_t port;
        ConnectionInfo(const std::string& addr, uint16_t p) : address(addr), port(p) 
        {
            // std::cout << "ConnectionInfo()" << std::endl;
        }

        // ConnectionInfo(const ConnectionInfo& conn) 
        // {
        //     // std::cout << 1 << std::endl;
        //     address = conn.address;
        //     port = conn.port;
        // }
        // ConnectionInfo& operator=(const ConnectionInfo& conn) 
        // {
        //     address = conn.address;
        //     port = conn.port;
        //     return *this;
        // }
        
        asio::awaitable<size_t> send_receive_data_async(asio::io_context& context, const std::vector<uint8_t>& data, std::vector<uint8_t>& packet_buffer) {
            asio::ip::tcp::socket socket(context);
            asio::ip::tcp::resolver resolver(context);
            asio::ip::tcp::resolver::results_type endpoints =
                    co_await resolver.async_resolve(address, std::to_string(port), asio::use_awaitable);

            co_await asio::async_connect(socket, endpoints, asio::use_awaitable);
            co_await asio::async_write(socket, asio::buffer(data), asio::use_awaitable);

            std::vector<uint8_t> buffer(Packet::max_packet_size);
            packet_buffer.clear();
            int expected_size = 0;
            size_t bytes_transferred = 0;

            try {
                for (;;) {    
                    if (expected_size != 0)
                    buffer.resize(expected_size - packet_buffer.size());
                    bytes_transferred = co_await socket.async_read_some(asio::buffer(buffer), asio::use_awaitable);
                    packet_buffer.insert(packet_buffer.end(), buffer.begin(), buffer.begin() + bytes_transferred);
                    
                    while (true)
                    {
                        if (packet_buffer.size() < Packet::header_size)
                            break;

                        if (!expected_size)
                            expected_size = Packet::get_packet_size(packet_buffer.data(), packet_buffer.size());


                        if (packet_buffer.size() < expected_size)
                            break;
                        co_return packet_buffer.size(); 
                    }
                }
            } catch (std::exception& e) {
                // socket.close();
                SPDLOG_ERROR("Error during send_receive_data_async: {}", e.what());
                throw std::runtime_error(std::format("receive_data: {}", e.what()));
            }
            co_return -1;
        }

        static std::vector<ConnectionInfo<Packet>> read_server_file(const std::string& server_file)
        {
            std::vector<Utils::ConnectionInfo<Packet>> connections;
            std::ifstream file(server_file);
            
            if (!file.is_open()) {
                throw std::runtime_error("Could not open server file: " + server_file);
            }

            std::string line;
            while (std::getline(file, line)) {
                if (line.empty() || line[0] == '#') continue; // Skip empty lines and comments
                size_t pos = line.find(':');
                if (pos == std::string::npos) continue; // Invalid line format
                std::string address = line.substr(0, pos);
                uint16_t port = static_cast<uint16_t>(std::stoi(line.substr(pos + 1)));
                connections.emplace_back(address, port);
            }

            file.close();
            return connections;
        }
    };

}

#endif