#ifndef FILE_MNGR_HPP
#define FILE_MNGR_HPP

#include "utils.hpp"
#include "cache_protocol.hpp"
#include <dirent.h>

namespace FileMngr {
    std::string set_local_file(const std::string& path, mode_t mode);
    void set_local_dir(const std::string& path, const std::string& meta_path, mode_t mode);
    
    std::string get_local_file(const std::string& path);
    std::string get_local_dir(const std::string& path, const std::string& meta_path, bool update_dir_list=false);

    int rmdir_recursive(const char* path);
    void remove_local_file(const std::string& path);
    void remove_local_dir(const std::string& path, const std::string& meta_path);

    void chmod_file(const std::string& path, Stat& file_proto);
    void chown_file(const std::string& path, Stat& file_proto);
    void rename_file(const std::string& path, Stat& file_proto);
    std::string update_local_file(const std::string& path, const UpdateCommand&);
    std::string update_local_dir(const std::string& path, const std::string& meta_path, const UpdateCommand&);
}

#endif