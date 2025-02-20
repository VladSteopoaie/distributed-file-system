#include "file_mngr.hpp"

std::string FileMngr::set_local_file(const std::string& file_path, mode_t mode)
{
    SPDLOG_DEBUG(std::format("Path: {}", file_path));
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

std::string FileMngr::set_local_dir(const std::string& dir_path, mode_t mode)
{
    SPDLOG_DEBUG(std::format("Path: {}", dir_path));
    std::string result_string;
    Stat dir_proto;
    struct stat dir_stat;
    int result;

    result = mkdir(dir_path.c_str(), mode);
    if (result != 0)
        throw std::runtime_error(std::format("set_local_dir: {}", std::strerror(errno)));

    if (stat(dir_path.c_str(), &dir_stat) != 0)
        throw std::runtime_error(std::format("set_local_dir: {}", std::strerror(errno)));

    Utils::struct_stat_to_proto(&dir_stat, dir_proto);
    dir_proto.SerializeToString(&result_string);


    return result_string;
}


std::string FileMngr::get_local_file(const std::string& file_path)
{
    SPDLOG_DEBUG(std::format("Path: {}", file_path));
    std::ifstream file(file_path);

    if (!file)
        throw std::runtime_error(std::format("get_local_file: {}", std::strerror(errno)));

    std::ostringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

std::string FileMngr::get_local_dir(const std::string& dir_path)
{
    Stat dir_proto;
    struct stat dir_stat;
    struct dirent* entry;
    SPDLOG_DEBUG(std::format("Path: {}", dir_path));
    DIR* dir = opendir(dir_path.c_str());
    std::string result;

    if (!dir)
        throw std::runtime_error(std::format("get_local_dir: {}", std::strerror(errno)));

    if (stat(dir_path.c_str(), &dir_stat) != 0)
        throw std::runtime_error(std::format("get_local_dir: {}", std::strerror(errno)));

    Utils::struct_stat_to_proto(&dir_stat, dir_proto);

    errno = 0;
    while ((entry = readdir(dir)) != nullptr)
        dir_proto.add_dir_list(entry->d_name);

    if (errno != 0)
        throw std::runtime_error(std::format("get_local_dir: {}", std::strerror(errno)));


    dir_proto.SerializeToString(&result);

    return result;
}


void FileMngr::remove_local_file(const std::string& path)
{
    int res = unlink(path.c_str());
    if (res != 0)
        throw std::runtime_error(std::format("remove_local_dir: {}", std::strerror(errno)));
}

void FileMngr::remove_local_dir(const std::string& path)
{
    int res = rmdir(path.c_str());
    if (res != 0)
        throw std::runtime_error(std::format("remove_local_dir: {}", std::strerror(errno)));
}

std::string FileMngr::update_local_file(const std::string& path, const UpdateCommand& command)
{
    try {
        Stat file_proto;
        std::string file_content = get_local_file(path);
        file_proto.ParseFromString(file_content);

        switch (UpdateCode::from_byte(command.opcode))
        {
            case UpdateCode::Type::CHMOD:
                // chmod_file(path, file_proto, command.argv);
                break;
            case UpdateCode::Type::CHOWN:
                // chown_file(path, file_proto, command.argv);
                break;
            case UpdateCode::Type::RENAME:
                // rename_file(path, file_proto, command.argv);
                break;
            default:
                throw std::runtime_error("Unknown update command.");
        }
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(std::format("update_local_file: {}", e.what()));
    }
}

std::string FileMngr::update_local_dir(const std::string& path, const UpdateCommand& command)
{
    int res = rmdir(path.c_str());
    if (res != 0)
        throw std::runtime_error(std::format("remove_local_dir: {}", std::strerror(errno)));
}