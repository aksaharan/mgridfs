#include "fs_meta_ops.h"
#include "fs_conn_info.h"
#include "fs_options.h"
#include "fs_logger.h"
#include "dir_meta_ops.h"

#include <iostream>
#include <mongo/client/gridfs.h>
#include <mongo/client/connpool.h>

using namespace std;
using namespace mongo;

bool mgridfs::mgridfs_load_or_create_root() {
	DBClientConnection dbc(true);
	string errorMessage;
	if (!dbc.connect(globalFSOptions._hostAndPort, errorMessage)) {
		std::cout << "Failed to connect to the server {error: " << errorMessage << "}" << std::endl;
	}

	std::cout << "Connection to mongodb succeeded {ConnId: " << dbc.getConnectionId() 
			<< ", WireVersion: {Min: " << dbc.getMinWireVersion() << ", Max: " << dbc.getMaxWireVersion() << "}"
			<< ", IsConnected: " << dbc.isStillConnected() << ", SO-timeout: " << dbc.getSoTimeout()
			<< ", Type: " << (long)dbc.type() << "}" << std::endl;

	GridFS gridFS(dbc, globalFSOptions._db, globalFSOptions._collPrefix);
	GridFile gridFile = gridFS.findFile(BSON("filename" << "/" << "metadata.type" << "directory"));

	std::cout << "GridFile from query {Filename: " << gridFile.getFilename() << ", metadata: " << gridFile.getMetadata() << std::endl;
	if (!gridFile.exists()) {
		std::cout << "Root directory not found for mounting, will try to create one now with following credentials: "
				<< "{UID: " << geteuid() << ", GID: " << getegid() << ", mode: 700" << "}"
				<< std::endl;
		if (!mgridfs_create_directory(dbc, "/", 0700, geteuid(), getegid())) {
			std::cout << "Failed to create root for the mounted filesystem in MongoDB" << std::endl;
			return false;
		}

		GridFile gridFile1 = gridFS.findFile(BSON("filename" << "/" << "metadata.type" << "directory"));
		if (!gridFile1.exists()) {
			std::cout << "Tried creating and failed to create the root directory, will not proceed further with file system mount"
					<< std::endl;
			return false;
		}
	}

	return true;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 */
void* mgridfs::mgridfs_init(struct fuse_conn_info* conn) {
	trace() << "-> entering mgridfs_init(fuse_conn_info)" << endl;

	DBClientConnection* pDbc = new DBClientConnection(true);
	string errorMessage;
	if (!pDbc->connect(globalFSOptions._hostAndPort, errorMessage)) {
		fatal() << "Failed to connect to the server {error: " << errorMessage << "}. Will quit filesystem operations. "
				<< "You may need to unmount the filesystem manually." << std::endl;
		fuse_exit(NULL);
	}

	std::cout << "Connection to mongodb succeeded {ConnId: " << pDbc->getConnectionId() 
			<< ", WireVersion: {Min: " << pDbc->getMinWireVersion() << ", Max: " << pDbc->getMaxWireVersion() << "}"
			<< ", IsConnected: " << pDbc->isStillConnected() << ", SO-timeout: " << pDbc->getSoTimeout()
			<< ", Type: " << (long)pDbc->type() << "}" << std::endl;

	trace() << "<- leaving mgridfs_init(fuse_conn_info)" << endl;
	return reinterpret_cast<void*>(pDbc);
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 */
void mgridfs::mgridfs_destroy(void* data) {
	trace() << "-> entering mgridfs_destroy(fuse_conn_info)" << endl;

	delete reinterpret_cast<DBClientConnection*>(data);
	trace() << "<- leaving mgridfs_destroy(fuse_conn_info)" << endl;
}

/** Get file system statistics
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 *
 * Replaced 'struct statfs' parameter with 'struct statvfs' in
 * version 2.5
 */
int mgridfs::mgridfs_statfs(const char *file, struct statvfs *statEntry) {
	trace() << "-> entering mgridfs_statfs{file: " << file << "}" << endl;

	fuse_context* fuseContext = fuse_get_context();
	DBClientConnection* pDbc = reinterpret_cast<DBClientConnection*>(fuseContext->private_data);

	BSONObj retInfo;
	if (!pDbc->runCommand(globalFSOptions._db, BSON("dbstats" << 1), retInfo)) {
		fatal() << "Failed to get db.stats from server " << retInfo << endl;
		return -EIO;
	}

	// Assumes that the database is purely used for file system and does not consider effects
	// of other collections on the stats
	/* Example return value from db.stats() command from server
	 * Received db.stats from server { 
	 * 	db: "rest", 
	 * 	collections: 5, 
	 * 	objects: 2731, 
	 * 	avgObjSize: 368.470157451483, 
	 * 	dataSize: 1006292, 
	 * 	storageSize: 1077248, 
	 * 	numExtents: 5, 
	 * 	indexes: 5, 
	 * 	indexSize: 40880, 
	 * 	fileSize: 50331648, 
	 * 	nsSizeMB: 16, 
	 * 	dataFileVersion: { major: 4, minor: 5 }, 
	 * 	ok: 1.0 
	 * }
	 */
	trace() << "Received db.stats from server " << retInfo << " -- SS: " << retInfo.getIntField("storageSize") 
			<< " -- FS: " << retInfo.getIntField("fileSize") << " -- O: " << retInfo.getIntField("objects")
			<< endl;
	bzero(statEntry, sizeof(*statEntry));
	statEntry->f_bsize = 1;
	statEntry->f_frsize = 1;
	statEntry->f_blocks = retInfo.getIntField("fileSize");
	statEntry->f_bfree = statEntry->f_bavail = statEntry->f_blocks - retInfo.getIntField("storageSize");

	// objects, could be more fine tuned based on collection size instead of just objects
	// Do inode calculations relative to the file and the storage sizes for the database
	statEntry->f_files = retInfo.getIntField("objects");
	if (statEntry->f_bavail > 0 && statEntry->f_blocks > 0) {
		long totalObjects = static_cast<long>(statEntry->f_blocks * 1.0 / (statEntry->f_blocks - statEntry->f_bavail) * statEntry->f_files);
		statEntry->f_ffree = statEntry->f_favail = totalObjects - statEntry->f_files;
		statEntry->f_files = totalObjects;
	}

	statEntry->f_namemax = 1000;

	trace() << "<- leaving mgridfs_statfs" << endl;
	return 0;
}
