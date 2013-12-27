#include "dir_meta_ops.h"
#include "fs_logger.h"
#include "fs_options.h"
#include "utils.h"
#include "file_handle.h"

#include <errno.h>

#include <iostream>

bool mgridfs::mgridfs_create_directory(DBClientBase& dbc, const std::string& path, mode_t dirMode, uid_t dirUid, gid_t dirGid) {
	trace() << "-> entering mgridfs_create_directory{dir: " << path << ", mode: " << dirMode
			<< ", uid: " << dirUid << ", gid: " << dirGid << "}" << std::endl;
	dirMode |= S_IFDIR;

	GridFS gridFS(dbc, globalFSOptions._db, globalFSOptions._collPrefix);
	BSONObj fileObj = gridFS.storeFile("", 0, path);
	if (!fileObj.isValid()) {
		error() << "Failed to create a directory for {path: " << path << "}" << std::endl;
		return false;
	}

	trace() << "File System created object {ns: " << globalFSOptions._filesNS << ", object: " << fileObj.getOwned().toString() 
			<< "}" << std::endl;
	BSONElement fileObjId = fileObj.getField("_id");

	dbc.update(globalFSOptions._filesNS, BSON("_id" << fileObjId.OID()), BSON("$set" << BSON("metadata.type" << "directory"
			<< "metadata.filename" << mgridfs::getPathBasename(path)
			<< "metadata.directory" << mgridfs::getPathDirname(path)
			<< "metadata.lastUpdated" << jsTime()
			<< "metadata.uid" << dirUid
			<< "metadata.gid" << dirGid
			<< "metadata.mode" << dirMode)));

	trace() << "<- leaving mgridfs_create_directory" << std::endl;
	return true;
}

/** Create a directory 
 *
 * Note that the mode argument may not have the type specification
 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 * correct directory type bits use  mode|S_IFDIR
 * */
int mgridfs::mgridfs_mkdir(const char *path, mode_t mode) {
	trace() << "-> entering mgridfs_mkdir{dir: " << path << ", mode: " << std::oct << mode << "}" << endl;
	fuse_context* fuseContext = fuse_get_context();
	DBClientConnection* pDbc = reinterpret_cast<DBClientConnection*>(fuseContext->private_data);
	if (!mgridfs_create_directory(*pDbc, path, mode, fuseContext->uid, fuseContext->gid)) {
		return -EACCES;
	}

	trace() << "-> leaving mgridfs_mkdir" << endl;
	return 0;
}

/** Remove a directory */
int mgridfs::mgridfs_rmdir(const char *path) {
	trace() << "-> entering mgridfs_rmdir{dir: " << path << "}" << endl;

	//First check if there are any files under the directory and bail out if any 
	fuse_context* fuseContext = fuse_get_context();
	DBClientConnection* pDbc = reinterpret_cast<DBClientConnection*>(fuseContext->private_data);
	auto_ptr<DBClientCursor> pCursor = pDbc->query(globalFSOptions._filesNS, BSON("metadata.directory" << path));
	if (pCursor->more()) {
		// There are entries under this directory and it cannot be deleted
		trace() << "Found entries for specified directory." << endl;
		return -ENOTEMPTY;
	}
	pCursor.reset(NULL); // Let the system free up the cursor held by this auto_ptr

	GridFS gridFS(*pDbc, globalFSOptions._db, globalFSOptions._collPrefix);
	gridFS.removeFile(path);

	trace() << "-> leaving mgridfs_rmdir" << endl;
	return 0;
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
int mgridfs::mgridfs_opendir(const char *path, struct fuse_file_info *ffinfo) {
	trace() << "-> entering mgridfs_opendir{dir: " << path << "}" << endl;

	fuse_context* fuseContext = fuse_get_context();
	DBClientConnection* pDbc = reinterpret_cast<DBClientConnection*>(fuseContext->private_data);

	//TODO: Possible change the query to be only on the fs.files instead of doing
	//a GridFile query that may be expensive in case on calls for large files with
	//incorrect directory check causing DoS kind of scenario
	GridFS gridFS(*pDbc, globalFSOptions._db, globalFSOptions._collPrefix);
	GridFile gridFile = gridFS.findFile(path);
	if (!gridFile.exists()) {
		debug() << "directory not found {path: " << path << "}" << endl;
		return -ENOENT;
	}

	BSONObj fileMeta = gridFile.getMetadata();
	int mode = fileMeta.getIntField("mode");
	if (!S_ISDIR(mode)) {
		return -ENOTDIR;
	}

	FileHandle fileHandle(path);
	ffinfo->fh = fileHandle.assignHandle();
	if (!ffinfo->fh) {
		return -ENFILE;
	}

	trace() << "<- leaving mgridfs_opendir" << endl;
	return 0;
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
int mgridfs::mgridfs_readdir(const char *path, void *dirlist, fuse_fill_dir_t ffdir, off_t offset, struct fuse_file_info *ffinfo) {
	trace() << "-> entering mgridfs_readdir{dir: " << path << ", offset: " << offset << "}" << endl;

	// Add meta directory links
	ffdir(dirlist, ".", NULL, 0);
	ffdir(dirlist, "..", NULL, 0);

	fuse_context* fuseContext = fuse_get_context();
	DBClientConnection* pDbc = reinterpret_cast<DBClientConnection*>(fuseContext->private_data);

	//TODO: Possible change the query to be only on the fs.files instead of doing
	//a GridFile query that may be expensive in case on calls for large files with
	//incorrect directory check causing DoS kind of scenario
	GridFS gridFS(*pDbc, globalFSOptions._db, globalFSOptions._collPrefix);
	auto_ptr<DBClientCursor> cursor = gridFS.list(BSON("metadata.directory" << path));
	while (cursor->more()) {
		// Catch for the AssertionException
		try {
			BSONObj obj = cursor->nextSafe();
			trace() << "iterating for directory " << obj.getStringField("filename") << endl;

			BSONElement elem = obj.getFieldDotted("metadata.filename");
			if (elem.ok() && !elem.String().empty()) {
				ffdir(dirlist, elem.String().c_str(), NULL, 0);
				trace() << "iterating for directory filename " << elem.String() << endl;
			} else if (strcmp("/", path)) {
				warn() << "Ignoring for missing metadata.filename property" << endl;
			}
		} catch (AssertionException& e) {
			error() << "Encountered exception in directory listing, will continue with next entry {exception: "
					<< e.what() << " : " << e.toString() << "}" << endl;
		}
	}

	// Add logic to list local not yet committed files as well in the directory
	// TODO: List the local files which are being written to temporary buffer until it
	// is committed and saved back to the GridFS database
	trace() << "<- leaving mgridfs_readdir" << endl;
	return 0;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int mgridfs::mgridfs_releasedir(const char *path, struct fuse_file_info *ffinfo) {
	trace() << "-> entering mgridfs_releasedir{dir: " << path << "}" << endl;

	FileHandle fileHandle(path);
	fileHandle.unassignHandle();

	trace() << "<- leaving mgridfs_releasedir" << endl;
	return 0;
}

/** Synchronize directory contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data
 *
 * Introduced in version 2.3
 */
int mgridfs::mgridfs_fsyncdir(const char *path, int param, struct fuse_file_info *ffinfo) {
	trace() << "-> entering mgridfs_fsyncdir{dir: " << path << ", param: " << param << "}" << endl;
	trace() << "<- leaving mgridfs_fsyncdir" << endl;
	//TODO: find out more about this call and implement appropriately
	return -EACCES;
}
