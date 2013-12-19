#ifndef mgridfs_fs_meta_ops_h
#define mgridfs_fs_meta_ops_h

#include <fuse.h>

namespace mgridfs {

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

}

#endif
