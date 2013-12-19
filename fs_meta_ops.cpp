#include "fs_meta_ops.h"
#include "fs_conn_info.h"
#include "fs_logger.h"

#include <iostream>
#include <mongo/client/gridfs.h>
#include <mongo/client/connpool.h>


/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 */
void* mgridfs::mgridfs_init(struct fuse_conn_info* conn) {
	trace() << "mgridfs_init(fuse_conn_info)" << endl;

	mgridfs::FSConnInfo* pConnInfo = new mgridfs::FSConnInfo();
	return pConnInfo;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 */
void mgridfs::mgridfs_destroy(void* data) {
	trace() << "mgridfs_destroy(fuse_conn_info)" << endl;

	delete reinterpret_cast<mgridfs::FSConnInfo*>(data);
}
