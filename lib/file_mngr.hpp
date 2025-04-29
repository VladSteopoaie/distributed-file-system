#ifndef FILE_MNGR_HPP
#define FILE_MNGR_HPP

#include "utils.hpp"
#include "net_protocol.hpp"
#include <dirent.h>

namespace FileMngr {
    std::string set_local_file(const std::string& path, mode_t mode);
    void set_local_dir(const std::string& path, const std::string& meta_path, mode_t mode);
    
    std::string get_local_file(const std::string& path);
    std::string get_local_dir(const std::string& path, const std::string& meta_path, bool update_dir_list=false);

    int rmdir_recursive(const char* path);
    void remove_local_file(const std::string& path);
    void remove_local_dir(const std::string& path, const std::string& meta_path);

    std::string chmod_object(const std::string& path, const std::vector<std::vector<uint8_t>>& argv);
    std::string chown_object(const std::string& path, const std::vector<std::vector<uint8_t>>& argv);
    std::string rename_object(const std::string& path, const std::string& dir, const std::vector<std::vector<uint8_t>>& argv);
    std::string update_local_object(const std::string& path, const std::string& file_metadata_dir, const std::string& dir_metadata_dir, const UpdateCommand& command, bool is_file);
    std::string update_local_file(const std::string& path, const std::string& file_metadata_dir, const UpdateCommand& command);
    std::string update_local_dir(const std::string& path, const std::string& file_metadata_dir, const std::string& dir_metadata_dir, const UpdateCommand& command);
}

#endif