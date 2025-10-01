// mounting: ./fs mounting_point
// unmounting: fusermount3 -u mounting_point

#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <sys/stat.h>
#include <unistd.h>
// #include "../../lib/cache_client.hpp"
// #include "../../lib/storage_client.hpp"
#include "../lib/cache_client.hpp"
#include "../lib/storage_client.hpp"

CacheAPI::CacheClient cache_client;
StorageAPI::StorageClient storage_client(128 * 1024);
std::ofstream read_log_file("/mnt/tmpfs/fs_read.log", std::ios::app);
std::ofstream write_log_file("/mnt/tmpfs/fs_write.log", std::ios::app);
std::ofstream cache_log_file("/mnt/tmpfs/fs_cache.log", std::ios::app);

struct HostInfo {
	std::string storage_address, storage_port;
	std::string cache_address, cache_port;

	HostInfo() : storage_address(""), storage_port(""), cache_address(""), cache_port("") {}
};


static int myfs_getattr(const char *path, struct stat *stat_buf, struct fuse_file_info *file_info)
{	
	(void) file_info;
	Stat proto;
	memset(stat_buf, 0, sizeof(struct stat));

	std::string proto_str = cache_client.get_file(path);
	if (proto_str.empty())
		proto_str = cache_client.get_dir(path);
		if (proto_str.empty())
			return -ENOENT;		

	proto.ParseFromString(proto_str);
	Utils::proto_to_struct_stat(proto, stat_buf);
	return 0;
}

static int myfs_mkdir(const char *path, mode_t mode)
{
	int error = cache_client.set_dir(path, std::to_string(mode));

	if (error < 0)
	{
		return -EIO;
	}

	return -error;
}

static int myfs_unlink(const char *path)
{
	// not a perfect error handling, but good enough for now
	int error_cache = cache_client.remove_file(path);
	int error_storage = storage_client.remove(path);
	if (error_cache < 0 && error_storage < 0)
	{
		return -EIO;
	}

	return 0;
}

static int myfs_rmdir(const char *path)
{
	int error = cache_client.remove_dir(path);

	if (error < 0)
	{
		return -EIO;
	}

	return -error;
}

static int myfs_rename(const char *old_path, const char *new_path, unsigned int flags)
{
	int error = cache_client.rename(old_path, new_path);

	if (error < 0)
	{
		return -EIO;
	}

	return 0;
}

static int myfs_chmod(const char *path, mode_t mode, struct fuse_file_info *file_info)
{
	int error = cache_client.chmod(path, mode);

	if (error < 0)
	{
		return -EIO;
	}

	return 0;
}

static int myfs_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *file_info)
{
	int error = cache_client.chown(path, uid, gid);

	if (error < 0)
	{
		return -EIO;
	}

	return 0;
}

static int myfs_open(const char *path, struct fuse_file_info *file_info)
{
	(void) file_info;
	std::string res = cache_client.get_file(path);
	if (res.empty())
		return -ENOENT;
	return 0;
}

int myfs_release(const char *path, struct fuse_file_info *file_info)
{
	(void) path;
	(void) file_info;

	return 0;
}

static int myfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *file_info)
{
	std::cout << "Read" << std::endl;
	std::cout << "Size: " << size << std::endl;
	std::cout << "Offset: " << offset << std::endl;
		
	// Utils::PerformanceTimer timer("Storage Client Read", read_log_file);
	size_t r_size = storage_client.read(path, buffer, size, offset);	
	// std::cout << "Request offset: " << offset << " received: " << r_size << std::endl;
	return r_size;
}

static int myfs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info * file_info)
{
	std::cout << "Write" << std::endl;
	std::cout << "Size: " << size << std::endl;
	std::cout << "Offset: " << offset << std::endl;

	// std::vector<uint8_t> vec_buffer(buffer, buffer + size);

	// return storage_client.write_stripes(path, vec_buffer, size, offset);
	// Utils::PerformanceTimer timer("Storage Client Write", write_log_file);
	int nbytes;
	// {
	nbytes = storage_client.write(path, buffer, size, offset);
	// }
	// std::cout << nbytes << std::endl;
	{
		// Utils::PerformanceTimer timer("Cache Client Chsize", cache_log_file);
		cache_client.chsize(path, offset + (off_t) nbytes);
	}
	return nbytes;
}

static int myfs_opendir(const char *path, struct fuse_file_info *file_info) 
{
	std::string result = cache_client.get_dir(path);
	if (result.empty())
		return -ENOENT;
	return 0;
}

static int myfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *file_info, enum fuse_readdir_flags flags)
{
	(void) offset;
	(void) file_info;
	(void) flags;

	Stat dir_proto;
	std::string dir_proto_str = cache_client.get_dir(path);
	if (dir_proto_str.empty())
		return -ENOENT;

	dir_proto.ParseFromString(dir_proto_str);

	for (const auto& entry : dir_proto.dir_list())
		filler(buffer, entry.c_str(), nullptr, 0, FUSE_FILL_DIR_PLUS);

	return 0;
}

static int myfs_releasedir(const char *path, struct fuse_file_info *file_info)
{
	(void) path;
	(void) file_info;
	return 0;
}

static void* myfs_init(struct fuse_conn_info *connection_info, struct fuse_config *config)
{
	(void) connection_info;
	// connection_info->max_write = 1024 * 128;
    // connection_info->max_read  = 1024 * 128;

	HostInfo *host_info = (struct HostInfo*) fuse_get_context()->private_data;
	if (host_info->cache_address.length() > 0 && host_info->cache_port.length() > 0)
	{
		cache_client.connect(host_info->cache_address, host_info->cache_port);
	}
	else
	{
		cache_client.connect("127.0.0.1", "8888"); 
	}

	if (host_info->storage_address.length() > 0 && host_info->storage_port.length() > 0)
	{
		storage_client.connect(host_info->storage_address, host_info->storage_port);
	}
	else
	{
		storage_client.connect("127.0.0.1", "13337"); 
	}
	return nullptr;
}

static int myfs_create(const char *path, mode_t mode, struct fuse_file_info *file_info) 
{
	(void) file_info;

	int error = cache_client.set_file(path, std::to_string(mode));

	if (error < 0)
	{
		fprintf(stderr, "Error: failed to create the file, please verify the logs!");
		return -EIO;
	}
	
	return -error;
}

static int myfs_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *file_info)
{
	(void) path;
	(void) tv;
	(void) file_info;
	return 0;
}

static const struct fuse_operations myfs_oper = {
	.getattr	= myfs_getattr,
	.mkdir 		= myfs_mkdir,
	.unlink		= myfs_unlink,
	.rmdir		= myfs_rmdir,
	.rename		= myfs_rename,
	.chmod		= myfs_chmod, 
	.chown		= myfs_chown, 
	.open		= myfs_open,
	.read		= myfs_read,
    .write 	    = myfs_write,
	.release	= myfs_release,
	.opendir	= myfs_opendir,
	.readdir	= myfs_readdir,
	.releasedir = myfs_releasedir,
	.init       = myfs_init,
	.create 	= myfs_create,
	.utimens	= myfs_utimens,
};

int main(int argc, char** argv)
{
	int ret;
	HostInfo host_info;
	struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
	spdlog::set_level(spdlog::level::debug); // Set global log level
	spdlog::set_pattern("(%s:%#) [%^%l%$] %v");
	// Parse command-line arguments
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "--cache-address") == 0 && i + 1 < argc) {
            host_info.cache_address = argv[i + 1];
            i++; 
        }
        else if (strcmp(argv[i], "--cache-port") == 0 && i + 1 < argc) {
            host_info.cache_port = argv[i + 1];
            i++; 
        }
        else if (strcmp(argv[i], "--storage-address") == 0 && i + 1 < argc) {
            host_info.storage_address = argv[i + 1];
            i++; 
        }
        else if (strcmp(argv[i], "--storage-port") == 0 && i + 1 < argc) {
            host_info.storage_port = argv[i + 1];
            i++; 
        }
		else {
			fuse_opt_add_arg(&args, argv[i]);
		}
    }

	ret = fuse_main(args.argc, args.argv, &myfs_oper, &host_info);
	fuse_opt_free_args(&args);
	
    return ret;
}
