#include "fs_options.h"
#include "fs_meta_ops.h"
#include "file_meta_ops.h"
#include "dir_meta_ops.h"

#include <unistd.h>
#include <iostream>
#include <fuse.h>

static struct fuse_operations mgridfsOps = {};
mgridfs::FSOptions mgridfs::globalFSOptions;

int main(int argc, char* argv[], char* arge[]) {
	std::cout << "MongoDB-GridFS" << std::endl;

	mgridfsOps.init = mgridfs::mgridfs_init;
	mgridfsOps.destroy = mgridfs::mgridfs_destroy;

	mgridfsOps.getattr = mgridfs::mgridfs_getattr;
	mgridfsOps.readlink = mgridfs::mgridfs_readlink;
	mgridfsOps.mknod = mgridfs::mgridfs_mknod;

	mgridfsOps.getdir = mgridfs::mgridfs_getdir;
	mgridfsOps.mkdir = mgridfs::mgridfs_mkdir;
	mgridfsOps.rmdir = mgridfs::mgridfs_rmdir;
	mgridfsOps.opendir = mgridfs::mgridfs_opendir;
	mgridfsOps.readdir = mgridfs::mgridfs_readdir;
	mgridfsOps.releasedir = mgridfs::mgridfs_releasedir;
	mgridfsOps.fsyncdir = mgridfs::mgridfs_fsyncdir;

	mgridfsOps.unlink = mgridfs::mgridfs_unlink;
	mgridfsOps.symlink = mgridfs::mgridfs_symlink;
	mgridfsOps.rename = mgridfs::mgridfs_rename;
	mgridfsOps.link = mgridfs::mgridfs_link;
	mgridfsOps.chmod = mgridfs::mgridfs_chmod;
	mgridfsOps.chown = mgridfs::mgridfs_chown;
	mgridfsOps.truncate = mgridfs::mgridfs_truncate;
	mgridfsOps.utime = mgridfs::mgridfs_utime;
	mgridfsOps.open = mgridfs::mgridfs_open;
	mgridfsOps.read = mgridfs::mgridfs_read;
	mgridfsOps.write = mgridfs::mgridfs_write;
	mgridfsOps.statfs = mgridfs::mgridfs_statfs;
	mgridfsOps.flush = mgridfs::mgridfs_flush;
	mgridfsOps.release = mgridfs::mgridfs_release;
	mgridfsOps.fsync = mgridfs::mgridfs_fsync;
	mgridfsOps.setxattr = mgridfs::mgridfs_setxattr;
	mgridfsOps.getxattr = mgridfs::mgridfs_getxattr;
	mgridfsOps.listxattr = mgridfs::mgridfs_listxattr;
	mgridfsOps.removexattr = mgridfs::mgridfs_removexattr;
	mgridfsOps.access = mgridfs::mgridfs_access;
	mgridfsOps.create = mgridfs::mgridfs_create;
	mgridfsOps.ftruncate = mgridfs::mgridfs_ftruncate;
	mgridfsOps.fgetattr = mgridfs::mgridfs_fgetattr;
	mgridfsOps.lock = mgridfs::mgridfs_lock;
	mgridfsOps.utimens = mgridfs::mgridfs_utimens;
	mgridfsOps.bmap = mgridfs::mgridfs_bmap;
	mgridfsOps.ioctl = mgridfs::mgridfs_ioctl;
	mgridfsOps.poll = mgridfs::mgridfs_poll;
	mgridfsOps.write_buf = mgridfs::mgridfs_write_buf;
	mgridfsOps.read_buf = mgridfs::mgridfs_read_buf;
	mgridfsOps.flock = mgridfs::mgridfs_flock;
	mgridfsOps.fallocate = mgridfs::mgridfs_fallocate;

	struct fuse_args fuseArgs = FUSE_ARGS_INIT(argc, argv);
	if (!mgridfs::globalFSOptions.fromCommandLine(fuseArgs)) {
		std::cerr << "ERROR: Failed to parse options passed to program, will not mount file system" << std::endl;
		return 1;
	}

	fuse_main(fuseArgs.argc, fuseArgs.argv, &mgridfsOps, NULL);
	return 0;
}
