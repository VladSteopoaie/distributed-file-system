// mounting: ./fs mounting_point
// unmounting: fusermount3 -u mounting_point

#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <sys/stat.h>
#include <unistd.h>
#include "lib/cache_client.hpp"

CacheAPI::CacheClient cache_client;

static void* myfs_init(struct fuse_conn_info *connection_info, struct fuse_config *config)
{
	// memcached connection
	// metadata_server connection
	cache_client.connect("127.0.0.1", "8888"); // temporary, address and port will be passed as arguments
	// storage_servers connection
	return nullptr;
}

static int myfs_getattr(const char *path, struct stat *stat_buf, struct fuse_file_info *file_info)
{
	int res = 0;

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
	// .chmod		= myfs_chmod,
	// .chown		= myfs_chown,
	// .release	= myfs_release,

	// // file specific
	// .open		= myfs_open,
	// .read		= myfs_read,
    // .write      = myfs_write,
	// .create 	= myfs_create,
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
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);


	ret = fuse_main(args.argc, args.argv, &myfs_oper, NULL);
	fuse_opt_free_args(&args);
	
    return ret;
}