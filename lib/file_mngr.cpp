#include "file_mngr.hpp"

std::string FileMngr::set_local_file(const std::string& file_path, mode_t mode)
{
    std::string result;
    Stat file_proto;
    struct stat file_stat;
    int fd;

    fd = open(file_path.c_str(), O_CREAT | O_RDWR, mode);
    if (fd < 0)
        throw std::runtime_error(std::format("set_local_file: {}", std::strerror(errno)));

    if (fstat(fd, &file_stat) != 0)
        throw std::runtime_error(std::format("set_local_file: {}", std::strerror(errno)));

    Utils::struct_stat_to_proto(&file_stat, file_proto);
    file_proto.SerializeToString(&result);
    if (write(fd, result.c_str(), result.length()) < 0)
        throw std::runtime_error(std::format("set_local_file: {}", std::strerror(errno)));

    return result;
}

void FileMngr::set_local_dir(const std::string& path, const std::string& meta_path, mode_t mode)
{
    int result;

    result = mkdir(path.c_str(), mode);
    if (result != 0)
        throw std::runtime_error(std::format("set_local_dir: {}", std::strerror(errno)));
    result = mkdir(meta_path.c_str(), mode);
    if (result != 0)
        throw std::runtime_error(std::format("set_local_dir: {}", std::strerror(errno)));
}


std::string FileMngr::get_local_file(const std::string& path)
{
    std::ifstream file(path);

    if (!file)
        throw std::runtime_error(std::format("get_local_file: {}", std::strerror(errno)));

    std::ostringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

std::string FileMngr::get_local_dir(const std::string& path, const std::string& meta_path, bool update_dir_list)
{
    std::string meta_dir_file = meta_path + "/.this"; // file that contains metadata of directory
    std::string result;
    if (update_dir_list == false)
    {
        std::ifstream i_file(meta_dir_file); // looking for metadat in .this
        if (i_file)
        {
            result = std::string((std::istreambuf_iterator<char>(i_file)), std::istreambuf_iterator<char>());
            i_file.close();
            return result;
        }
    }

    // if we are doing an update we have to make sure that no previous data remains
    // this is in case there is an error before writing to .this
    unlink(meta_dir_file.c_str());

    Stat dir_proto;
    struct stat dir_stat;
    struct dirent* entry;
    std::ofstream o_file;

    DIR* dir = opendir(path.c_str());

    if (!dir)
        throw std::runtime_error(std::format("get_local_dir: {}", std::strerror(errno)));

    if (stat(path.c_str(), &dir_stat) != 0)
        throw std::runtime_error(std::format("get_local_dir: {}", std::strerror(errno)));

    Utils::struct_stat_to_proto(&dir_stat, dir_proto);

    errno = 0; // does not work without this line \('_')/
    while ((entry = readdir(dir)) != nullptr)
        dir_proto.add_dir_list(entry->d_name);

    if (errno != 0)
        throw std::runtime_error(std::format("get_local_dir: {}", std::strerror(errno)));

    dir_proto.SerializeToString(&result);

    o_file.open(meta_dir_file);
    if (!o_file.is_open())
       SPDLOG_ERROR(std::format("get_local_dir: {}", std::strerror(errno)));
    
    o_file << result;
    o_file.close();

    return result;
} // get_local_dir

int FileMngr::rmdir_recursive(const char* path)
{
    struct dirent *entry;
    DIR *dir = opendir(path);

    if (!dir) {
        return -1;
    }

    char filepath[1024];
    struct stat statbuf;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);

        if (stat(filepath, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
            rmdir_recursive(filepath);  // recursively delete subdirectory
        } else {
            unlink(filepath);  // delete file
        }
    }

    closedir(dir);
    int res = rmdir(path);
    return res;
} // rmdir_recursive

void FileMngr::remove_local_file(const std::string& path)
{
    int res = unlink(path.c_str());
    if (res != 0)
        throw std::runtime_error(std::format("remove_local_dir: {}", std::strerror(errno)));
}

void FileMngr::remove_local_dir(const std::string& path, const std::string& meta_path)
{

    int res = rmdir(path.c_str());
    if (res != 0)
        throw std::runtime_error(std::format("remove_local_dir: {}", std::strerror(errno)));
    res = rmdir_recursive(meta_path.c_str());
    if (res != 0)
        throw std::runtime_error(std::format("remove_local_dir: {}", std::strerror(errno)));
}

std::string FileMngr::chmod_object(const std::string& path, const std::vector<std::vector<uint8_t>>& argv)
{
    try 
    {
        std::string content = get_local_file(path);

        Stat proto;
        proto.ParseFromString(content);
        mode_t new_mode = Utils::get_int_from_byte_array(argv[0]);
        proto.set_mode(new_mode);
        proto.SerializeToString(&content);

        std::ofstream o_file(path);
        if (o_file.is_open())
            o_file << content;
        else
            throw std::runtime_error(std::strerror(errno));
        
        return content;
    }
    catch (std::exception& e)
    {   
        throw std::runtime_error(std::format("chmod_object: {}", e.what()));
    }
}

std::string FileMngr::chown_object(const std::string& path, const std::vector<std::vector<uint8_t>>& argv)
{
    try 
    {
        std::string content = get_local_file(path);

        Stat proto;
        proto.ParseFromString(content);
        uid_t new_uid = Utils::get_int_from_byte_array(argv[0]);
        gid_t new_gid = Utils::get_int_from_byte_array(argv[1]);
        proto.set_uid(new_uid);
        proto.set_gid(new_gid);
        proto.SerializeToString(&content);

        std::ofstream o_file(path);
        if (o_file.is_open())
            o_file << content;
        else
            throw std::runtime_error(std::strerror(errno));
        
        return content;
    }
    catch (std::exception& e)
    {   
        throw std::runtime_error(std::format("chmod_object: {}", e.what()));
    }

}

std::string FileMngr::rename_object(const std::string& path, const std::string& meta_dir, const std::vector<std::vector<uint8_t>>& argv)
{
    std::string new_path = meta_dir + Utils::get_string_from_byte_array(argv[0]);
    std::string old_path = meta_dir + path;
    int res = rename(old_path.c_str(), new_path.c_str());
    if (res != 0)
        throw std::runtime_error(std::format("rename_object: {}", std::strerror(errno)));
    return Utils::get_string_from_byte_array(argv[0]);
}

std::string FileMngr::update_local_object(const std::string& path, const std::string& file_metadata_dir, const std::string& dir_metadata_dir, const UpdateCommand& command, bool is_file)
{
    try {
        std::string file_meta = file_metadata_dir + path;
        std::string dir_meta = dir_metadata_dir + path;
        std::string content;
        switch (UpdateCode::from_byte(command.opcode))
        {
            case UpdateCode::Type::CHMOD:
                if (is_file)
                    content = chmod_object(file_meta, command.argv);
                else 
                    content = chmod_object(dir_meta + "/.this", command.argv);
                break;
            case UpdateCode::Type::CHOWN:
                if (is_file)
                    content = chown_object(file_meta, command.argv);
                else 
                    content = chown_object(dir_meta + "/.this", command.argv);
                break;
            case UpdateCode::Type::RENAME:
                content = rename_object(path, file_metadata_dir, command.argv);
                if (!is_file)
                    rename_object(path, dir_metadata_dir, command.argv);    
                break;
            default:
                throw std::runtime_error("Unknown update command.");
        }

        return content;
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(std::format("update_local_file: {}", e.what()));
    }
}


std::string FileMngr::update_local_file(const std::string& path, const std::string& file_metadata_dir, const UpdateCommand& command)
{
    try 
    {
        return update_local_object(path, file_metadata_dir, "", command, true);
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(e.what());
    }
}

std::string FileMngr::update_local_dir(const std::string& path, const std::string& file_metadata_dir, const std::string& dir_metadata_dir, const UpdateCommand& command)
{
    try 
    {
        return update_local_object(path, file_metadata_dir, dir_metadata_dir, command, false);
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(e.what());
    }
}