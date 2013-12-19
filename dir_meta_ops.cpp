#include "dir_meta_ops.h"
#include "fs_logger.h"

#include <errno.h>

#include <iostream>


/* Deprecated, use readdir() instead */
int mgridfs::mgridfs_getdir(const char *, fuse_dirh_t, fuse_dirfil_t) {
	error() << "-> mgridfs_getdir() not supported" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Create a directory 
 *
 * Note that the mode argument may not have the type specification
 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 * correct directory type bits use  mode|S_IFDIR
 * */
int mgridfs::mgridfs_mkdir(const char *dirname, mode_t mode) {
	trace() << "-> entering mgridfs_mkdir{dir: " << dirname << ", mode: " << mode << "}" << endl;
	trace() << "-> leaving mgridfs_mkdir{dir: " << dirname << ", mode: " << mode << "}" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Remove a directory */
int mgridfs::mgridfs_rmdir(const char *dirname) {
	trace() << "-> entering mgridfs_rmdir{dir: " << dirname << "}" << endl;
	trace() << "-> leaving mgridfs_rmdir{dir: " << dirname << "}" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Open directory
 *
 * Unless the 'default_permissions' mount option is given,
 * this method should check if opendir is permitted for this
 * directory. Optionally opendir may also return an arbitrary
 * filehandle in the fuse_file_info structure, which will be
 * passed to readdir, closedir and fsyncdir.
 *
 * Introduced in version 2.3
 */
int mgridfs::mgridfs_opendir(const char *dirname, struct fuse_file_info *ffinfo) {
	trace() << "-> entering mgridfs_opendir{dir: " << dirname << "}" << endl;
	trace() << "<- leaving mgridfs_opendir{dir: " << dirname << "}" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int mgridfs::mgridfs_readdir(const char *dirname, void *dirlist, fuse_fill_dir_t ffdir, off_t offset, struct fuse_file_info *ffinfo) {
	trace() << "-> entering mgridfs_readdir{}" << endl;
	trace() << "<- leaving mgridfs_readdir{}" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int mgridfs::mgridfs_releasedir(const char *dirname, struct fuse_file_info *ffinfo) {
	trace() << "-> entering mgridfs_releasedir{}" << endl;
	trace() << "<- leaving mgridfs_releasedir{}" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Synchronize directory contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data
 *
 * Introduced in version 2.3
 */
int mgridfs::mgridfs_fsyncdir(const char *dirname, int param, struct fuse_file_info *ffinfo) {
	trace() << "-> entering mgridfs_fsyncdir{}" << endl;
	trace() << "<- leaving mgridfs_fsyncdir{}" << endl;
	errno = ENOTSUP;
	return -1;
}
