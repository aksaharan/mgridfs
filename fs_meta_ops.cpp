#include "fs_meta_ops.h"
#include "fs_options.h"
#include "fs_logger.h"
#include "dir_meta_ops.h"

#include <iostream>
#include <mongo/client/gridfs.h>
#include <mongo/client/connpool.h>

using namespace std;
using namespace mongo;

int mgridfs::mgridfs_load_or_create_root() {

	try {
		ScopedDbConnection dbc(globalFSOptions._connectString);

		info() << "Connection to mongodb succeeded {ConnId: " << dbc.conn().getConnectionId() 
			<< ", WireVersion: {Min: " << dbc.conn().getMinWireVersion() << ", Max: " << dbc.conn().getMaxWireVersion() << "}"
			<< ", IsConnected: " << dbc.conn().isStillConnected() << ", SO-timeout: " << dbc.conn().getSoTimeout()
			<< ", Type: " << (long)dbc.conn().type() << "}" << std::endl;

		GridFS gridFS(dbc.conn(), globalFSOptions._db, globalFSOptions._collPrefix);
		GridFile gridFile = gridFS.findFile(BSON("filename" << "/" << "metadata.type" << "directory"));

		debug() << "GridFile from query {Filename: " << gridFile.getFilename() << ", metadata: " << gridFile.getMetadata() << std::endl;
		if (!gridFile.exists()) {
			info() << "Root directory not found for mounting, will try to create one now with following credentials: "
				<< "{UID: " << geteuid() << ", GID: " << getegid() << ", mode: 700" << "}"
				<< std::endl;
			int retValue = mgridfs_create_directory(dbc.conn(), "/", 0700, geteuid(), getegid());
			if (retValue) {
				error() << "Failed to create root for the mounted filesystem in MongoDB" << std::endl;
				dbc.done();
				return -EIO;
			}

			GridFile gridFile1 = gridFS.findFile(BSON("filename" << "/" << "metadata.type" << "directory"));
			if (!gridFile1.exists()) {
				error() << "Tried creating and failed to create the root directory, will not proceed further with file system mount"
					<< std::endl;
				dbc.done();
				return -ENOENT;
			}
		}
		dbc.done();
	} catch (DBException& e) {
		error() << "Caught exception in processing {code: " << e.getCode() << ", what: " << e.what()
			<< ", exception: " << e.toString() << "}" << endl;
		return -EIO;
	}

	return 0;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 */
void* mgridfs::mgridfs_init(struct fuse_conn_info* conn) {
	trace() << "-> requested mgridfs_init(fuse_conn_info)" << endl;
	return NULL;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 */
void mgridfs::mgridfs_destroy(void* data) {
	trace() << "-> requested mgridfs_destroy(fuse_conn_info)" << endl;
}

/** Get file system statistics
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 *
 * Replaced 'struct statfs' parameter with 'struct statvfs' in
 * version 2.5
 */
int mgridfs::mgridfs_statfs(const char *file, struct statvfs *statEntry) {
	trace() << "-> requested mgridfs_statfs{file: " << file << "}" << endl;

	BSONObj retInfo;

	try {
		ScopedDbConnection dbc(globalFSOptions._connectString);
		if (!dbc->runCommand(globalFSOptions._db, BSON("dbstats" << 1), retInfo)) {
			fatal() << "Failed to get db.stats from server " << retInfo << endl;
			return -EIO;
		}

		dbc.done();
	} catch (DBException& e) {
		error() << "Caught exception in processing {code: " << e.getCode() << ", what: " << e.what()
			<< ", exception: " << e.toString() << "}" << endl;
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
	BSONElement ssElem = retInfo.getField("storageSize");
	BSONElement fsElem = retInfo.getField("fileSize");
	BSONElement oElem = retInfo.getField("objects");
	trace() << "Received db.stats from server " << retInfo << " -- SS: " << ssElem.numberLong() 
			<< " -- FS: " << fsElem.numberLong() << " -- O: " << oElem.numberLong()
			<< endl;
	bzero(statEntry, sizeof(*statEntry));
	statEntry->f_bsize = 1;
	statEntry->f_frsize = 1;
	statEntry->f_blocks = fsElem.numberLong();
	statEntry->f_bfree = statEntry->f_bavail = statEntry->f_blocks - ssElem.numberLong();

	// objects, could be more fine tuned based on collection size instead of just objects
	// Do inode calculations relative to the file and the storage sizes for the database
	statEntry->f_files = oElem.numberLong();
	if (statEntry->f_bavail > 0 && statEntry->f_blocks > 0) {
		long totalObjects = static_cast<long>(statEntry->f_blocks * 1.0 / (statEntry->f_blocks - statEntry->f_bavail) * statEntry->f_files);
		statEntry->f_ffree = statEntry->f_favail = totalObjects - statEntry->f_files;
		statEntry->f_files = totalObjects;
	}

	statEntry->f_namemax = 1000;
	return 0;
}
