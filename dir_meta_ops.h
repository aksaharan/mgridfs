#ifndef mgridfs_dir_meta_ops_h
#define mgridfs_dir_meta_ops_h

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fuse.h>


namespace mgridfs {

/* Deprecated, use readdir() instead */
int mgridfs_getdir(const char *, fuse_dirh_t, fuse_dirfil_t);

/** Create a directory 
 *
 * Note that the mode argument may not have the type specification
 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 * correct directory type bits use  mode|S_IFDIR
 * */
int mgridfs_mkdir(const char *, mode_t);

/** Remove a directory */
int mgridfs_rmdir(const char *);

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
int mgridfs_opendir(const char *, struct fuse_file_info *);

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
int mgridfs_readdir(const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *);

/** Release directory
 *
 * Introduced in version 2.3
 */
int mgridfs_releasedir(const char *, struct fuse_file_info *);

/** Synchronize directory contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data
 *
 * Introduced in version 2.3
 */
int mgridfs_fsyncdir(const char *, int, struct fuse_file_info *);

}

#endif
