// mounting: ./fs mounting_point
// unmounting: fusermount3 -u mounting_point

#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../lib/cache_client.hpp"

CacheAPI::CacheClient cache_client;

struct HostInfo {
	std::string address, port;

	HostInfo() : address(""), port("") {}
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
	int error = cache_client.remove_file(path);

	if (error < 0)
	{
		return -EIO;
	}

	return -error;
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
	return 0;
}


static int myfs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info * file_info)
{           
	return 0;
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
	HostInfo *host_info = (struct HostInfo*) fuse_get_context()->private_data;
	if (host_info)
		cache_client.connect(host_info->address, host_info->port);
	else
		cache_client.connect("127.0.0.1", "8888"); 
	// storage_servers connection
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
	.rename		= myfs_rename, // todo
	.chmod		= myfs_chmod, // todo
	.chown		= myfs_chown, // todo
	.open		= myfs_open,
	// .read		= myfs_read,
    // .write 	    = myfs_write,
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
	spdlog::set_level(spdlog::level::info); // Set global log level to debug
	spdlog::set_pattern("(%s:%#) [%^%l%$] %v");
	// Parse command-line arguments
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "--address") == 0 && i + 1 < argc) {
            host_info.address = argv[i + 1];
            i++; // Skip the next argument
        }
        else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            host_info.port = argv[i + 1];
            i++; // Skip the next argument
        }
		else {
			fuse_opt_add_arg(&args, argv[i]);
		}
    }

	ret = fuse_main(args.argc, args.argv, &myfs_oper, &host_info);
	fuse_opt_free_args(&args);
	
    return ret;
}