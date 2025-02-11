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
	std::string str_mode = std::to_string(mode);

	int error = cache_client.set(path, str_mode);

	if (error == 0)
		return 0;
	else if (error < 0)
	{
		fprintf(strerr, "Error: failed to create the file, please verify the logs!")
		return -EIO;
	}
	else 
		return -error;
}

static int myfs_getattr(const char *path, struct stat *stat_buf, struct fuse_file_info *file_info)
{	
	(void) file_info;
	memset(stat_buf, 0, sizeof(struct stat));

	std::string value = cache_client.get(path);
	if (value == "")
		return -ENOENT;

	
	return 0;
	// ask memcached with key == path
	// receive response

	// if no file 
		// then:
			// ask metadata_server for the file
			// receive response
			// if no file
				// then: res = -ENOENT
				// else: copy stat
		// else:
			// copy stat 

	return res;
}

static int myfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *file_info)
{
	return 0;
}

static int myfs_open(const char *path, struct fuse_file_info *file_info)
{
	return 0;

}

static int myfs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info * file_info)
{           

	return 0;
}

static int myfs_mkdir(const char *path, mode_t mode)
{

	return 0;
}

static int myfs_readdir(const char *path, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *, enum fuse_readdir_flags)
{
	return 0;

}

static const struct fuse_operations myfs_oper = {
	.init       = myfs_init,
	// .getattr	= myfs_getattr,
	// .rename 	= myfs_rename,
	// .chmod	= myfs_chmod,
	// .chown	= myfs_chown,
	// .release	= myfs_release,

	// // file specific
	// .open		= myfs_open,
	// .read		= myfs_read,
    // .write 	    = myfs_write,
	.create 		= myfs_create,
	// .unlink		= myfs_unlink,

	// // dir specific
	// .mkdir 		= myfs_mkdir,
	// .readdir	= myfs_readdir,
	// .opendir	= myfs_opendir,
	// .rmdir		= myfs_rmdir,
	// .releasedir = myfs_releasedir
};

int main(int argc, char** argv)
{
	int ret;
	HostInfo host_info;
	struct fuse_args args = FUSE_ARGS_INIT(0, NULL);

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