#ifndef mgridfs_fs_meta_ops_h
#define mgridfs_fs_meta_ops_h

#include <fuse.h>

namespace mgridfs {

/**
 * Pre-load / create initial directory
 *
 * This is a non-fuse method to pre-load or create the initial directory on mount.
 * This would allow us to error out before suceeding mount if there are any issues.
 */
int mgridfs_load_or_create_root();

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 */
void* mgridfs_init(struct fuse_conn_info* conn);

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 */
void mgridfs_destroy(void* data);

/** Get file system statistics
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 *
 * Replaced 'struct statfs' parameter with 'struct statvfs' in
 * version 2.5
 */
int mgridfs_statfs(const char *, struct statvfs *);

}

#endif
