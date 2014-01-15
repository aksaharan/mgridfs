#include "file_meta_ops.h"
#include "fs_options.h"
#include "fs_logger.h"
#include "utils.h"
#include "file_handle.h"
#include "local_gridfs.h"
#include "local_grid_file.h"

#include <string.h>
#include <errno.h>

#include <algorithm>
#include <iostream>

#include <mongo/client/gridfs.h>
#include <mongo/client/connpool.h>

using namespace std;
using namespace mongo;

namespace {
	// All static definitions used by the meta-functions
	const string METADATA_XATTR_PREFIX = "metadata.xattr.";
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.	 The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int mgridfs::mgridfs_getattr(const char* file, struct stat* file_stat) {
	trace() << "-> requested mgridfs_getattr{file: " << file << "}" << endl;

	try {
		ScopedDbConnection dbc(globalFSOptions._connectString);
		GridFS gridFS(dbc.conn(), globalFSOptions._db, globalFSOptions._collPrefix);
		GridFile gridFile = gridFS.findFile(BSON("filename" << file));
		dbc.done();

		if (!gridFile.exists()) {
			debug() << "Requested file not found for attribute listing {file: " << file << "}" << endl;
			return -ENOENT;
		}

		BSONObj fileMeta = gridFile.getMetadata();
		debug() << "File Meta " << fileMeta << "... and lastUpdated: " << fileMeta.getField("lastUpdated") << endl;

		bzero(file_stat, sizeof(*file_stat));
		file_stat->st_uid = fileMeta.hasField("uid") ? fileMeta.getIntField("uid") : 1;
		file_stat->st_gid = fileMeta.hasField("gid") ? fileMeta.getIntField("gid") : 1;
		file_stat->st_mode = fileMeta.hasField("mode") ? fileMeta.getIntField("mode") : 0555;

		file_stat->st_ctime = gridFile.getUploadDate().toTimeT();
		if (fileMeta.hasField("lastUpdated")) {
			file_stat->st_mtime = fileMeta.getField("lastUpdated").Date().toTimeT();
		} else {
			file_stat->st_mtime = file_stat->st_ctime;
		}

		file_stat->st_nlink = 1;
		if (S_ISDIR(file_stat->st_mode)) {
			file_stat->st_nlink++;
			file_stat->st_size = fileMeta.objsize();
			file_stat->st_blocks = get512BlockCount(file_stat->st_size);
		} else if (S_ISREG(file_stat->st_mode)) {
			file_stat->st_size = gridFile.getContentLength();
			file_stat->st_blocks = get512BlockCount(file_stat->st_size);
		} else if (S_ISLNK(file_stat->st_mode)) {
			const char* targetLink = fileMeta.getStringField("target");
			if (targetLink) {
				file_stat->st_size = strlen(targetLink);
				file_stat->st_blocks = get512BlockCount(file_stat->st_size);
			} else {
				warn() << "Encountered a NULL target field for a link" << endl;
			}
		} else {
			warn() << "Encountered unsupported file stat mode for the entry " << gridFile.getMetadata()
				<< " -> {filename: " << gridFile.getFilename() << "}" << endl;
		}
	} catch (DBException& e) {
		error() << "Caught exception in processing {code: " << e.getCode() << ", what: " << e.what()
				<< ", exception: " << e.toString() << "}" << endl;
		return -EIO;
	}

	return 0;
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
int mgridfs::mgridfs_fgetattr(const char *file, struct stat *stats, struct fuse_file_info *ffinfo) {
	trace() << "-> requested mgridfs_fgetattr{file: " << file << ", fh: " << ffinfo->fh << "}" << endl;

	FileHandle fileHandle(file, ffinfo->fh);
	if (!fileHandle.isValid()) {
		return -EBADF;
	}

	return mgridfs_getattr(fileHandle.getFilename().c_str(), stats);
}

/** Create a file node
 *
 * This is called for creation of all non-directory, non-symlink
 * nodes.  If the filesystem defines a create() method, then for
 * regular files that will be called instead.
 */
int mgridfs::mgridfs_mknod(const char *file, mode_t mode, dev_t dev) {
	trace() << "-> requested mgridfs_mknod{file: " << file << ", mode: " << std::oct << mode << ", dev: " << std::dec << dev << "}" << endl;

	// TODO: Implement this for some of the types like regular files / named-fifo etc.
	// This won't be supported for any other special file / device type

	return -ENOTSUP;
}

/** Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.	If the linkname is too long to fit in the
 * buffer, it should be truncated.	The return value should be 0
 * for success.
 */
int mgridfs::mgridfs_readlink(const char *file, char *link, size_t len) {
	trace() << "-> requested mgridfs_readlink{file: " << file << ", len: " << len << "}" << endl;
	if (len <= 0) {
		return -EINVAL;
	}

	try {
		ScopedDbConnection dbc(globalFSOptions._connectString);
		GridFS gridFS(dbc.conn(), globalFSOptions._db, globalFSOptions._collPrefix);
		GridFile gridFile = gridFS.findFile(BSON("filename" << file));
		dbc.done();

		if (!gridFile.exists()) {
			debug() << "Requested file not found for symlink listing {file: " << file << "}" << endl;
			return -ENOENT;
		}

		BSONObj fileMeta = gridFile.getMetadata();
		const char* targetLink = fileMeta.getStringField("target");
		if (targetLink) {
			strncpy(link, targetLink, len - 1);
			link[len - 1] = 0;
		} else {
			link[0] = 0;
			warn() << "Encountered a NULL target field for a link, will return empty value" << endl;
		}
	} catch (DBException& e) {
		error() << "Caught exception in processing {code: " << e.getCode() << ", what: " << e.what()
			<< ", exception: " << e.toString() << "}" << endl;
		return -EIO;
	}

	return 0;
}

/** Remove a file */
int mgridfs::mgridfs_unlink(const char *file) {
	trace() << "-> requested mgridfs_unlink{file: " << file << "}" << endl;

	try {
		ScopedDbConnection dbc(globalFSOptions._connectString);
		GridFS gridFS(dbc.conn(), globalFSOptions._db, globalFSOptions._collPrefix);
		gridFS.removeFile(file);
		dbc.done();

	} catch (DBException& e) {
		error() << "Caught exception in processing {code: " << e.getCode() << ", what: " << e.what()
			<< ", exception: " << e.toString() << "}" << endl;
		return -EIO;
	}

	return 0;
}

/** Create a symbolic link */
int mgridfs::mgridfs_symlink(const char *srcfile, const char *destfile) {
	trace() << "-> requested mgridfs_symlink{srcfile: " << srcfile << ", destfile: " << destfile << "}" << endl;

	try {
		fuse_context* fuseContext = fuse_get_context();
		ScopedDbConnection dbc(globalFSOptions._connectString);
		GridFS gridFS(dbc.conn(), globalFSOptions._db, globalFSOptions._collPrefix);
		BSONObj fileObj = gridFS.storeFile("", 0, destfile);
		if (!fileObj.isValid()) {
			error() << "Failed to create link file {destfile: " << destfile << "}" << std::endl;
			return -EIO;
		}

		debug() << "File System created link object {ns: " << globalFSOptions._filesNS << ", object: " << fileObj.getOwned().toString() 
			<< "}" << std::endl;

		mode_t linkMode = S_IFLNK | S_IRWXU | S_IRWXG | S_IRWXO;
		BSONElement fileObjId = fileObj.getField("_id");
		dbc->update(globalFSOptions._filesNS, BSON("_id" << fileObjId.OID()), BSON("$set" << BSON("metadata.type" << "slink"
						<< "metadata.target" << srcfile
						<< "metadata.filename" << mgridfs::getPathBasename(destfile)
						<< "metadata.directory" << mgridfs::getPathDirname(destfile)
						<< "metadata.lastUpdated" << jsTime()
						<< "metadata.uid" << fuseContext->uid
						<< "metadata.gid" << fuseContext->gid
						<< "metadata.mode" << linkMode)));
		dbc.done();
	} catch (DBException& e) {
		error() << "Caught exception in processing {code: " << e.getCode() << ", what: " << e.what()
			<< ", exception: " << e.toString() << "}" << endl;
		return -EIO;
	}

	return 0;
}

/** Rename a file */
int mgridfs::mgridfs_rename(const char *srcfile, const char *destfile) {
	trace() << "-> requested mgridfs_rename{srcfie: " << srcfile << ", destfile: " << destfile << "}" << endl;

	return -ENOTSUP;
}

/** Create a hard link to a file */
int mgridfs::mgridfs_link(const char *srcfile, const char *destfile) {
	trace() << "-> requested mgridfs_link{srcfile: " << srcfile << ", destfile: " << destfile << "}" << endl;

	info() << "hard-links are not supported {srcfile: " << srcfile << ", destfile: " << destfile << "}" << endl;
	return -ENOTSUP;
}

/** Change the permission bits of a file */
int mgridfs::mgridfs_chmod(const char *file, mode_t mode) {
	trace() << "-> requested mgridfs_chmod{file: " << file << ", mode: " << std::oct << mode << "}" << endl;
	try {
		ScopedDbConnection dbc(globalFSOptions._connectString);
		dbc->update(globalFSOptions._filesNS, BSON("filename" << file), BSON("$set" << BSON("metadata.mode" << mode)));
		BSONObj errorDetail = dbc->getLastErrorDetailed();
		dbc.done();

		int n = errorDetail.getIntField("n");
		if (n <= 0) {
			debug() << "Failed to chmod requested file {file: " << file << "}" << endl;
			return -ENOENT;
		}
	} catch (DBException& e) {
		error() << "Caught exception in processing {code: " << e.getCode() << ", what: " << e.what()
			<< ", exception: " << e.toString() << "}" << endl;
		return -EIO;
	}

	return 0;
}

/** Change the owner and group of a file */
int mgridfs::mgridfs_chown(const char *file, uid_t uid, gid_t gid) {
	trace() << "-> requested mgridfs_chown{file: " << file << ", uid: " << uid << ", gid: " << gid << "}" << endl;

	try {
		ScopedDbConnection dbc(globalFSOptions._connectString);
		dbc->update(globalFSOptions._filesNS, BSON("filename" << file), BSON("$set" << BSON("metadata.uid" << uid << "metadata.gid" << gid)));
		BSONObj errorDetail = dbc->getLastErrorDetailed();
		dbc.done();

		int n = errorDetail.getIntField("n");
		if (n <= 0) {
			debug() << "Failed to chown requested file {file: " << file << "}" << endl;
			return -ENOENT;
		}
	} catch (DBException& e) {
		error() << "Caught exception in processing {code: " << e.getCode() << ", what: " << e.what()
			<< ", exception: " << e.toString() << "}" << endl;
		return -EIO;
	}

	return 0;
}

/** Change the size of a file */
int mgridfs::mgridfs_truncate(const char *file, off_t len) {
	trace() << "-> requested mgridfs_truncate{file: " << file << ", len: " << len << "}" << endl;
	LocalGridFile* localGridFile = LocalGridFS::get().findByName(file);
	if (!localGridFile) {
		error() << "Should have found a local file for truncate operation to happen on it {file: "
			<< file << "}" << endl;
		return -EBADF;
	}

	if (!localGridFile->setSize(len)) {
		error() << "Failed to set specified size for the local file {file: " << file
			<< ", offset: " << len << "}" << endl;
		return -EIO;
	}

	return 0;
}

/** Change the access and/or modification times of a file
 *
 * Deprecated, use utimens() instead.
 */
int mgridfs::mgridfs_utime(const char *file, struct utimbuf *time) {
	trace() << "-> requested mgridfs_utime{file: " << file << "}" << endl;

	Date_t updateTime;
	if (time) {
		// Convert seconds to milli seconds
		updateTime = Date_t((unsigned long long)time->modtime * 1000);
	} else {
		updateTime = jsTime();
	}

	try {
		ScopedDbConnection dbc(globalFSOptions._connectString);
		dbc->update(globalFSOptions._filesNS, BSON("filename" << file), BSON("$set" << BSON("metadata.lastUpdated" << updateTime)));
		BSONObj errorDetail = dbc->getLastErrorDetailed();
		dbc.done();

		int n = errorDetail.getIntField("n");
		if (n <= 0) {
			debug() << "Failed to utime requested file {file: " << file << "}" << endl;
			return -ENOENT;
		}
	} catch (DBException& e) {
		error() << "Caught exception in processing {code: " << e.getCode() << ", what: " << e.what()
			<< ", exception: " << e.toString() << "}" << endl;
		return -EIO;
	}

	return 0;
}

/** File open operation
 *
 * No creation (O_CREAT, O_EXCL) and by default also no
 * truncation (O_TRUNC) flags will be passed to open(). If an
 * application specifies O_TRUNC, fuse first calls truncate()
 * and then open(). Only if 'atomic_o_trunc' has been
 * specified and kernel version is 2.6.24 or later, O_TRUNC is
 * passed on to open.
 *
 * Unless the 'default_permissions' mount option is given,
 * open should check if the operation is permitted for the
 * given flags. Optionally open may also return an arbitrary
 * filehandle in the fuse_file_info structure, which will be
 * passed to all file operations.
 *
 * Changed in version 2.2
 */
int mgridfs::mgridfs_open(const char *file, struct fuse_file_info *ffinfo) {
	trace() << "-> requested mgridfs_open{file: " << file << ", fh: " << ffinfo->fh << ", flags: " << ffinfo->flags << "}" << endl;

	// Assign a new file handle for this open
	// TODO: check behaviour on the changing a read-only file descriptor to read-write descriptor
	FileHandle fileHandle(file, 0);
	ffinfo->fh = fileHandle.assignHandle();
	if (!ffinfo->fh) {
		// Failed to generate a file handle, most likely out of resource
		return -ENFILE;
	}

	// First check if this is one of the local files being written currently
	// If so, it can be opened in read / write modes
	// TODO: check for handling additional modes likes truncate / append etc
	LocalGridFile* localGridFile = LocalGridFS::get().findByName(file);
	if (localGridFile) {
		return 0;
	}

	// For now, support in basic read-only mode to get started
	debug() << "Flags {AccessFlags: " << (ffinfo->flags & O_ACCMODE) << ", ReadOnly: " <<  ((ffinfo->flags & O_ACCMODE) & O_RDONLY)
			<< ", AccessMask: " << O_ACCMODE << ", ROMask: " << O_RDONLY << "}" << endl;
	try {
		ScopedDbConnection dbc(globalFSOptions._connectString);
		GridFS gridFS(dbc.conn(), globalFSOptions._db, globalFSOptions._collPrefix);
		GridFile gridFile = gridFS.findFile(file);
		dbc.done();

		//TODO: do error checking for local file creation
		if (gridFile.exists() && ((ffinfo->flags & O_ACCMODE) == O_RDONLY)) {
			// Do not need local file, read-only data should be read from the server directly until someone else on this
			// server is writing data
			return 0;
		} else if (gridFile.exists() && ((ffinfo->flags & O_ACCMODE) != O_RDONLY)) {
			// Create local file and let it open with data from the server in certain cases
			LocalGridFile* localGridFile = LocalGridFS::get().createFile(file);
			if (!localGridFile) {
				return -ENOMEM;
			}

			int retCode = localGridFile->openRemote(ffinfo->flags);
			if (retCode != 0) {
				LocalGridFS::get().releaseFile(file);
				return -EIO;
			}

			if (ffinfo->flags & O_TRUNC && !localGridFile->setSize(0)) {
				error() << "Truncate file flag enabled and failed to set local file size to 0 {filename: "
					<< file << ", truncEnabled: " << (ffinfo->flags & O_TRUNC) << "}" << endl;
				return -EIO;
			}
			return 0;
		} else if (!gridFile.exists() && (ffinfo->flags & O_CREAT)) {
			// Create remote file and open local file for the same
			fileHandle.unassignHandle(); // Unassign the handle since a new handle will be assigned in the create call

			fuse_context* fuseContext = fuse_get_context();
			return mgridfs_create(file, fuseContext->umask, ffinfo);
		} else {
			// Following conditions are not met for it to be true:
			// 	- Exists and requested in RO mode
			// 	- Exists and requested in RW / WO mode
			// 	- Not exists and requested to be created
			fileHandle.unassignHandle();
			return -ENOENT;
		}
	} catch (DBException& e) {
		error() << "Caught exception in processing {code: " << e.getCode() << ", what: " << e.what()
			<< ", exception: " << e.toString() << "}" << endl;
		fileHandle.unassignHandle();
		return -EIO;
	}

	//TODO: Support files in read/write mode as well for existing files in the GridFS
	fileHandle.unassignHandle();
	return -EACCES;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.	 An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
int mgridfs::mgridfs_read(const char *file, char *data, size_t len, off_t offset, struct fuse_file_info *ffinfo) {
	trace() << "-> requested mgridfs_read{file: " << file << ", fh: " << ffinfo->fh << ", len: " << len << ", offset: " << offset << "}" << endl;

	if (len == 0) {
		return 0;
	}

	FileHandle fileHandle(file, ffinfo->fh);
	if (!fileHandle.isValid() || fileHandle.getFilename().empty()) {
		return -EBADF;
	}

	LocalGridFile* localGridFile = LocalGridFS::get().findByName(fileHandle.getFilename());
	if (localGridFile) {
		return localGridFile->read(data, len, offset);
	} else if ((ffinfo->flags & O_ACCMODE) != O_RDONLY) {
		warn() << "Local grid file not found for a read request for file opened in non-readonly mode." << endl;
		return -EBADF;
	}

	// If there is no local grid file in the scope, read appropriate data from GridFS directly and copy the same to the specified buffer
	try {
		ScopedDbConnection dbc(globalFSOptions._connectString);
		GridFS gridFS(dbc.conn(), globalFSOptions._db, globalFSOptions._collPrefix);
		GridFile gridFile = gridFS.findFile(BSON("filename" << file));
		if (!gridFile.exists()) {
			warn() << "Requested file not found for reading data {file: " << fileHandle.getFilename() << "}" << endl;
			dbc.done();
			return -EBADF;
		} else if (offset < 0 || offset >= (off_t)gridFile.getContentLength()) {
			// TODO: Fix the offset logic
			// EoF reached, return end-of-file 
			trace() << "Reached end-of-file for the specified request {file: " << fileHandle.getFilename() 
				<< ", offset: " << offset << "}" << endl;
			dbc.done();
			return 0;
		}

		// Else read the appropriate chunks from the server and copy into the buffer
		unsigned int chunkSize = gridFile.getChunkSize();
		unsigned int numChunks = gridFile.getNumChunks();
		//TODO: Implement the file offset tracking for the file
		//
		/*
		off_t startOffset = (offset < 0) ? offset + len : offset;
		off_t endOffset = (offset < 0) ? 0 : offset;
		*/
		unsigned int activeChunkNum = offset / chunkSize;
		size_t bytesRead = 0;
		while (bytesRead < len && activeChunkNum < numChunks) {
			GridFSChunk activeChunk = gridFile.getChunk(activeChunkNum);
			int bytesToRead = 0;
			int chunkLen = 0;
			const char* chunkData = activeChunk.data(chunkLen);
			if (!chunkData) {
				warn() << "Encountered NULL chunk data while reading file from remote server {activeChunkNum: " << activeChunkNum 
					<< "}, will return IO error to the reader." << endl;
				dbc.done();
				return -EIO;
			}

			if (bytesRead) {
				// If we are in-between reading data, check on what data is left to be read
				bytesToRead = min((size_t)chunkLen, (len - bytesRead));
				memcpy(data + bytesRead, chunkData, bytesToRead);
			} else {
				// This is the first chunk we are reading and could be starting from in-between offset of the
				// chunk that was previously read partially
				bytesToRead = min((size_t)(chunkLen - (offset % chunkSize)), (size_t)(len - bytesRead));
				memcpy(data + bytesRead, chunkData + (offset % chunkSize), bytesToRead);
			}

			bytesRead += bytesToRead;
			++activeChunkNum;
		}

		dbc.done();
		return bytesRead;

	} catch (DBException& e) {
		error() << "Caught exception in processing {code: " << e.getCode() << ", what: " << e.what()
				<< ", exception: " << e.toString() << "}" << endl;
		return -EIO;
	}

	return 0;
}

/** Store data from an open file in a buffer
 *
 * Similar to the read() method, but data is stored and
 * returned in a generic buffer.
 *
 * No actual copying of data has to take place, the source
 * file descriptor may simply be stored in the buffer for
 * later data transfer.
 *
 * The buffer must be allocated dynamically and stored at the
 * location pointed to by bufp.  If the buffer contains memory
 * regions, they too must be allocated using malloc().  The
 * allocated memory will be freed by the caller.
 *
 * Introduced in version 2.9
 */
int mgridfs::mgridfs_read_buf(const char *file, struct fuse_bufvec **bufp, size_t size, off_t offset, struct fuse_file_info *ffinfo) {
	//-> requested mgridfs_read_buf{file: /xxxxx.txt, fh: 51, size: 4096, offset: 0}
	trace() << "-> requested mgridfs_read_buf{file: " << file << ", fh: " << ffinfo->fh << ", size: " << size 
			<< ", offset: " << offset << "}" << endl;
	// mgridfs_read should be usable instead of this method
	return -ENOTSUP;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.	 An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
int mgridfs::mgridfs_write(const char *file, const char *data, size_t len, off_t offset, struct fuse_file_info *ffinfo) {
	trace() << "-> requested mgridfs_write{file: " << file << ", fh: " << ffinfo->fh << ", len: " << len << ", offset: " << offset << "}" << endl;

	FileHandle fileHandle(file, ffinfo->fh);
	if (!fileHandle.isValid()) {
		return -EBADF;
	}

	LocalGridFile* localGridFile = LocalGridFS::get().findByName(fileHandle.getFilename());
	if (!localGridFile) {
		return -EBADF;
	}

	return localGridFile->write(data, len, offset);
}

/** Write contents of buffer to an open file
 *
 * Similar to the write() method, but data is supplied in a
 * generic buffer.  Use fuse_buf_copy() to transfer data to
 * the destination.
 *
 * Introduced in version 2.9
 */
int mgridfs::mgridfs_write_buf(const char *file, struct fuse_bufvec *buf, off_t off, struct fuse_file_info *ffinfo) {
	trace() << "-> requested mgridfs_write_buf{file: " << file << ", fh: " << ffinfo->fh << ", offset: " << off << "}" << endl;
	// mgridfs_write should be usable instead of this method
	return -ENOTSUP;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().	This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.	It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
int mgridfs::mgridfs_flush(const char *file, struct fuse_file_info *ffinfo) {
	trace() << "-> requested mgridfs_flush{file: " << file << ", fh: " << ffinfo->fh << "}" << endl;

	FileHandle fileHandle(file, ffinfo->fh);
	if (!fileHandle.isValid()) {
		return -EBADF;
	}

	// If read-only mode file, this does not need to be flushed to the database and can be ignored safely
	if ((ffinfo->flags & O_ACCMODE) == O_RDONLY) {
		//Unassign the handle before returning
		//fileHandle.unassignHandle();
		return 0;
	}

	LocalGridFile* localGridFile = LocalGridFS::get().findByName(fileHandle.getFilename());
	if (!localGridFile) {
		return -EBADF;
	}

	return localGridFile->flush();
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.	 It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int mgridfs::mgridfs_release(const char *file, struct fuse_file_info *ffinfo) {
	trace() << "-> requested mgridfs_release{file: " << file << ", fh: " << ffinfo->fh << "}" << endl;

	FileHandle fileHandle(file, ffinfo->fh);
	if (fileHandle.isValid()) {
		// If there is still an active file handle mapping, go through all open file handles for 
		// the specified file name and make sure all the files are released / closed
		LocalGridFile* localGridFile = LocalGridFS::get().findByName(fileHandle.getFilename());
		if (localGridFile) {
			if (localGridFile->isDirty()) {
				localGridFile->flush();
			}

			LocalGridFS::get().releaseFile(fileHandle.getFilename());
		}

		/*
		// Release all handles associated with this filename
		FileHandle::unassignAllHandles(fileHandle.getFilename());
		*/
	}

	return 0;
}

/** Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 *
 * Changed in version 2.2
 */
int mgridfs::mgridfs_fsync(const char *file, int param, struct fuse_file_info *ffinfo) {
	trace() << "-> requested mgridfs_fsync{file: " << file << ", fh: " << ffinfo->fh << ", param: " << param << "}" << endl;

	return -ENOTSUP;
}

/** Set extended attributes */
int mgridfs::mgridfs_setxattr(const char *file, const char *name, const char *value, size_t len, int flags) {
	trace() << "-> requested mgridfs_setxattr{file: " << file << ", name: " << name << ", len: " << len << "}" << endl;
	//TODO: change the implementation
	return 0;
}

/** Get extended attributes */
int mgridfs::mgridfs_getxattr(const char *file, const char *name, char *value, size_t len) {
	trace() << "-> requested mgridfs_getxattr{file: " << file << ", name: " << name << ", len: " << len << "}" << endl;
	//TODO: change the implementation
	if (len > 0) {
		value[0] = '\0';
	}
	return 0;
}

/** List extended attributes */
int mgridfs::mgridfs_listxattr(const char *file, char *buffer, size_t len) {
	trace() << "-> requested mgridfs_listxattr{file: " << file << ", buflen: " << len << "}" << endl;
	//TODO: change the implementation
	// for now, do nothing and don't support any additional attributes
	if (len > 0) {
		buffer[0] = '\0';
	}
	return 0;
}

/** Remove extended attributes */
int mgridfs::mgridfs_removexattr(const char *file, const char *attr) {
	trace() << "-> requested mgridfs_removexattr{file: " << file << ", attr: " << attr << "}" << endl;
	if (globalFSOptions._metadataKeyMap.left.find(attr) == globalFSOptions._metadataKeyMap.left.end()) {
		// It is not one of the core attributes of thr GridFS file and will go under metadata.xattr object
		string xAttr = METADATA_XATTR_PREFIX + attr;
		debug() << "Will remove the extra attribute from the file {attr: " << attr 
				<< ", mappedAttr: " << xAttr << "}" << endl;
		//TODO: implementation for actual deletion here
	}

	// for now, do nothing and don't support any additional attributes
	return 0;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int mgridfs::mgridfs_create(const char *file, mode_t fileMode, struct fuse_file_info *ffinfo) {
	trace() << "-> requested mgridfs_create{file: " << file << ", fh: " << ffinfo->fh << ", mode: " << std::oct << fileMode << "}" << endl;
	// From man-page: creat() is equivalent to open() with flags equal to O_CREAT|O_WRONLY|O_TRUNC.
	fileMode |= S_IFREG;

	try {
		fuse_context* fuseContext = fuse_get_context();
		ScopedDbConnection dbc(globalFSOptions._connectString);
		GridFS gridFS(dbc.conn(), globalFSOptions._db, globalFSOptions._collPrefix);

		// Create an empty file to signify the file creation and open a local file for the same
		BSONObj fileObj = gridFS.storeFile("", 0, file);
		if (!fileObj.isValid()) {
			warn() << "Failed to create file for {path: " << file << "}" << std::endl;
			dbc.done();
			return -EBADF;
		}

		//TODO: Check if the file already exists both locally as well as remotely
		trace() << "File System created object {ns: " << globalFSOptions._filesNS << ", object: " << fileObj.getOwned().toString() 
			<< "}" << std::endl;
		BSONElement fileObjId = fileObj.getField("_id");

		dbc->update(globalFSOptions._filesNS, BSON("_id" << fileObjId.OID()), BSON("$set" << BSON("metadata.type" << "file"
						<< "metadata.filename" << mgridfs::getPathBasename(file)
						<< "metadata.directory" << mgridfs::getPathDirname(file)
						<< "metadata.lastUpdated" << jsTime()
						<< "metadata.uid" << fuseContext->uid
						<< "metadata.gid" << fuseContext->gid
						<< "metadata.mode" << fileMode)));
		dbc.done();

	} catch (DBException& e) {
		error() << "Caught exception in processing {code: " << e.getCode() << ", what: " << e.what()
				<< ", exception: " << e.toString() << "}" << endl;
		return -EIO;
	}

	// Now since the file is created on MongoDB GridFS, create a local cache file to represent the remote file
	FileHandle fileHandle(file, 0);
	ffinfo->fh = fileHandle.assignHandle();
	if (!ffinfo->fh) {
		// Most likely failed to generate file handle because it ran out of it.
		return -ENFILE;
	}

	LocalGridFile* localGridFile = LocalGridFS::get().createFile(file);
	if (!localGridFile) {
		fileHandle.unassignHandle();
		return -ENOMEM;
	}

	return 0;
}

/**
 * Change the size of an open file
 *
 * This method is called instead of the truncate() method if the
 * truncation was invoked from an ftruncate() system call.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the truncate() method will be
 * called instead.
 *
 * Introduced in version 2.5
 */
int mgridfs::mgridfs_ftruncate(const char *file, off_t offset, struct fuse_file_info *ffinfo) {
	trace() << "-> requested mgridfs_ftruncate{file: " << file << ", fh: " << ffinfo->fh << ", offset: " << offset << "}" << endl;
	FileHandle fileHandle(file, ffinfo->fh);
	if (!fileHandle.isValid()) {
		return -EBADF;
	}

	return mgridfs_truncate(fileHandle.getFilename().c_str(), offset);
}

/**
 * Perform POSIX file locking operation
 *
 * The cmd argument will be either F_GETLK, F_SETLK or F_SETLKW.
 *
 * For the meaning of fields in 'struct flock' see the man page
 * for fcntl(2).  The l_whence field will always be set to
 * SEEK_SET.
 *
 * For checking lock ownership, the 'fuse_file_info->owner'
 * argument must be used.
 *
 * For F_GETLK operation, the library will first check currently
 * held locks, and if a conflicting lock is found it will return
 * information without calling this method.	 This ensures, that
 * for local locks the l_pid field is correctly filled in.	The
 * results may not be accurate in case of race conditions and in
 * the presence of hard links, but it's unlikely that an
 * application would rely on accurate GETLK results in these
 * cases.  If a conflicting lock is not found, this method will be
 * called, and the filesystem may fill out l_pid by a meaningful
 * value, or it may leave this field zero.
 *
 * For F_SETLK and F_SETLKW the l_pid field will be set to the pid
 * of the process performing the locking operation.
 *
 * Note: if this method is not implemented, the kernel will still
 * allow file locking to work locally.  Hence it is only
 * interesting for network filesystems and similar.
 *
 * Introduced in version 2.6
 */
int mgridfs::mgridfs_lock(const char *path, struct fuse_file_info *ffinfo, int cmd, struct flock *fl) {
	trace() << "-> requested mgridfs_lock{file: " << path << ", fh: " << ffinfo->fh << ", cmd: " << cmd << "}" << endl;

	return -ENOTSUP;
}

/**
 * Change the access and modification times of a file with
 * nanosecond resolution
 *
 * This supersedes the old utime() interface.  New applications
 * should use this.
 *
 * See the utimensat(2) man page for details.
 *
 * Introduced in version 2.6
 */
int mgridfs::mgridfs_utimens(const char *file, const struct timespec tv[2]) {
	trace() << "-> requested mgridfs_utimens{file: " << file << "}" << endl;

	struct utimbuf tm = {};
	if (!tv[1].tv_sec) {
		tm.actime = tv[0].tv_sec;
		tm.modtime = tv[1].tv_sec;
	} else {
		tm.actime = tm.modtime = time(NULL);
	}

	return mgridfs_utime(file, &tm);
}

/**
 * Map block index within file to block index within device
 *
 * Note: This makes sense only for block device backed filesystems
 * mounted with the 'blkdev' option
 *
 * Introduced in version 2.6
 */
int mgridfs::mgridfs_bmap(const char *file, size_t blocksize, uint64_t *idx) {
	trace() << "-> requested mgridfs_bmap{file: " << file << ", blocksize: " << blocksize << "}" << endl;

	warn() << "Not a block device, bmap() is not supported." << endl;
	return -ENOTSUP;
}

/**
 * Ioctl
 *
 * flags will have FUSE_IOCTL_COMPAT set for 32bit ioctls in
 * 64bit environment.  The size and direction of data is
 * determined by _IOC_*() decoding of cmd.  For _IOC_NONE,
 * data will be NULL, for _IOC_WRITE data is out area, for
 * _IOC_READ in area and if both are set in/out area.  In all
 * non-NULL cases, the area is of _IOC_SIZE(cmd) bytes.
 *
 * Introduced in version 2.8
 */
int mgridfs::mgridfs_ioctl(const char *file, int cmd, void *arg, struct fuse_file_info *ffinfo, unsigned int flags, void *data) {
	trace() << "-> requested mgridfs_ioctl{file: " << file << ", fh: " << ffinfo->fh << ", cmd: " << cmd << "flags: " << flags << "}" << endl;

	return -ENOTSUP;
}

/**
 * Poll for IO readiness events
 *
 * Note: If ph is non-NULL, the client should notify
 * when IO readiness events occur by calling
 * fuse_notify_poll() with the specified ph.
 *
 * Regardless of the number of times poll with a non-NULL ph
 * is received, single notification is enough to clear all.
 * Notifying more times incurs overhead but doesn't harm
 * correctness.
 *
 * The callee is responsible for destroying ph with
 * fuse_pollhandle_destroy() when no longer in use.
 *
 * Introduced in version 2.8
 */
int mgridfs::mgridfs_poll(const char *file, struct fuse_file_info *ffinfo, struct fuse_pollhandle *ph, unsigned *reventsp) {
	trace() << "-> requested mgridfs_poll{file: " << file << ", fh: " << ffinfo->fh << "}" << endl;

	return -ENOTSUP;
}

/**
 * Perform BSD file locking operation
 *
 * The op argument will be either LOCK_SH, LOCK_EX or LOCK_UN
 *
 * Nonblocking requests will be indicated by ORing LOCK_NB to
 * the above operations
 *
 * For more information see the flock(2) manual page.
 *
 * Additionally fi->owner will be set to a value unique to
 * this open file.  This same value will be supplied to
 * ->release() when the file is released.
 *
 * Note: if this method is not implemented, the kernel will still
 * allow file locking to work locally.  Hence it is only
 * interesting for network filesystems and similar.
 *
 * Introduced in version 2.9
 */
int mgridfs::mgridfs_flock(const char *file, struct fuse_file_info *ffinfo, int op) {
	trace() << "-> requested mgridfs_flock{file: " << file << ", fh: " << ffinfo->fh << ", op: " << op << "}" << endl;

	return -ENOTSUP;
}

/**
 * Allocates space for an open file
 *
 * This function ensures that required space is allocated for specified
 * file.  If this function returns success then any subsequent write
 * request to specified range is guaranteed not to fail because of lack
 * of space on the file system media.
 *
 * Introduced in version 2.9.1
 */
int mgridfs::mgridfs_fallocate(const char *file, int mode, off_t offset, off_t len, struct fuse_file_info *ffinfo) {
	trace() << "-> requested mgridfs_fallocate{file: " << file << ", fh: " << ffinfo->fh << ", mode: " << std::oct << mode 
			<< ", offset: " << offset << ", len: " << len << "}" << endl;

	return -ENOTSUP;
}
