#include "file_meta_ops.h"
#include "fs_options.h"
#include "fs_logger.h"

#include <string.h>
#include <errno.h>

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
	trace() << "-> entering mgridfs_getattr{" << file << "}" << endl;
	DBClientConnection* pConn = reinterpret_cast<DBClientConnection*>(fuse_get_context()->private_data);
	if (!pConn) {
		error() << "Encountered null client connection value, will return error" << endl;
		return -EFAULT;
	}

	GridFS gridFS(*pConn, globalFSOptions._db, globalFSOptions._collPrefix);
	GridFile gridFile = gridFS.findFile(BSON("filename" << file));
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
	} else if (S_ISREG(file_stat->st_mode)) {
		file_stat->st_size = gridFile.getContentLength();
	} else {
		warn() << "Encountered unknown file stat mode for theh entry " << gridFile.getMetadata()
				<< " -> {filename: " << gridFile.getFilename() << "}" << endl;
	}

	trace() << "<- leaving mgridfs_getattr" << endl;
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
	trace() << "-> entering mgridfs_fgetattr{file: " << file << "}" << endl;
	int retValue = mgridfs_getattr(file, stats);
	trace() << "<- leaving mgridfs_fgetattr" << endl;
	return retValue;
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
	trace() << "-> entering mgridfs_readlink{file: " << file << ", len: " << len << "}" << endl;
	trace() << "<- leaving mgridfs_readlink" << endl;
	return -ENOTSUP;
}

/** Create a file node
 *
 * This is called for creation of all non-directory, non-symlink
 * nodes.  If the filesystem defines a create() method, then for
 * regular files that will be called instead.
 */
int mgridfs::mgridfs_mknod(const char *file, mode_t mode, dev_t dev) {
	trace() << "-> entering mgridfs_mknod{file: " << file << ", mode: " << std::oct << mode << ", dev: " << std::dec << dev << "}" << endl;
	// TODO: Implement this for some of the types like regular files / named-fifo etc.
	// This won't be supported for any other special file / device type
	trace() << "<- leaving mgridfs_mknod" << endl;
	return -ENOTSUP;
}

/** Remove a file */
int mgridfs::mgridfs_unlink(const char *file) {
	trace() << "-> entering mgridfs_unlink{file: " << file << "}" << endl;
	trace() << "<- leaving mgridfs_unlink" << endl;
	return -ENOTSUP;
}

/** Create a symbolic link */
int mgridfs::mgridfs_symlink(const char *srcfile, const char *destfile) {
	trace() << "-> entering mgridfs_symlink{srcfile: " << srcfile << ", destfile: " << destfile << "}" << endl;
	//TRACE -> entering mgridfs_symlink{srcfile: /home/ec2-user/source/mgridfs/mgridfs, destfile: /mgridfs}
	trace() << "<- leaving mgridfs_symlink" << endl;
	return -ENOTSUP;
}

/** Rename a file */
int mgridfs::mgridfs_rename(const char *srcfile, const char *destfile) {
	trace() << "-> entering mgridfs_rename{srcfie: " << srcfile << ", destfile: " << destfile << "}" << endl;
	trace() << "<- leaving mgridfs_rename" << endl;
	return -ENOTSUP;
}

/** Create a hard link to a file */
int mgridfs::mgridfs_link(const char *srcfile, const char *destfile) {
	trace() << "-> entering mgridfs_link{srcfile: " << srcfile << ", destfile: " << destfile << "}" << endl;
	trace() << "<- leaving mgridfs_link" << endl;
	return -ENOTSUP;
}

/** Change the permission bits of a file */
int mgridfs::mgridfs_chmod(const char *file, mode_t mode) {
	trace() << "-> entering mgridfs_chmod{file: " << file << ", mode: " << std::oct << mode << "}" << endl;
	DBClientConnection* pConn = reinterpret_cast<DBClientConnection*>(fuse_get_context()->private_data);
	if (!pConn) {
		error() << "Encountered null client connection value, will return error" << endl;
		return -EFAULT;
	}

	pConn->update(globalFSOptions._filesNS, BSON("filename" << file), BSON("$set" << BSON("metadata.mode" << mode)));
	BSONObj errorDetail = pConn->getLastErrorDetailed();
	int n = errorDetail.getIntField("n");
	if (n <= 0) {
		debug() << "Failed to chmod requested file {file: " << file << "}" << endl;
		return -ENOENT;
	}

	trace() << "<- leaving mgridfs_chmod" << endl;
	return 0;
}

/** Change the owner and group of a file */
int mgridfs::mgridfs_chown(const char *file, uid_t uid, gid_t gid) {
	trace() << "-> entering mgridfs_chown{file: " << file << ", uid: " << uid << ", gid: " << gid << "}" << endl;

	DBClientConnection* pConn = reinterpret_cast<DBClientConnection*>(fuse_get_context()->private_data);
	if (!pConn) {
		error() << "Encountered null client connection value, will return error" << endl;
		return -EFAULT;
	}

	pConn->update(globalFSOptions._filesNS, BSON("filename" << file), BSON("$set" << BSON("metadata.uid" << uid << "metadata.gid" << gid)));
	BSONObj errorDetail = pConn->getLastErrorDetailed();
	int n = errorDetail.getIntField("n");
	if (n <= 0) {
		debug() << "Failed to chown requested file {file: " << file << "}" << endl;
		return -ENOENT;
	}

	trace() << "<- leaving mgridfs_chown" << endl;
	return 0;
}

/** Change the size of a file */
int mgridfs::mgridfs_truncate(const char *file, off_t len) {
	trace() << "-> entering mgridfs_truncate{file: " << file << ", len: " << len << "}" << endl;
	trace() << "<- leaving mgridfs_truncate" << endl;
	return -ENOTSUP;
}

/** Change the access and/or modification times of a file
 *
 * Deprecated, use utimens() instead.
 */
int mgridfs::mgridfs_utime(const char *file, struct utimbuf *time) {
	trace() << "-> entering mgridfs_utime{file: " << file << "}" << endl;
	DBClientConnection* pConn = reinterpret_cast<DBClientConnection*>(fuse_get_context()->private_data);
	if (!pConn) {
		error() << "Encountered null client connection value, will return error" << endl;
		return -EFAULT;
	}

	Date_t updateTime;
	if (time) {
		// Convert seconds to milli seconds
		updateTime = Date_t((unsigned long long)time->modtime * 1000);
	} else {
		updateTime = jsTime();
	}

	pConn->update(globalFSOptions._filesNS, BSON("filename" << file), BSON("$set" << BSON("metadata.lastUpdated" << updateTime)));
	BSONObj errorDetail = pConn->getLastErrorDetailed();
	int n = errorDetail.getIntField("n");
	if (n <= 0) {
		debug() << "Failed to utime requested file {file: " << file << "}" << endl;
		return -ENOENT;
	}
	trace() << "<- leaving mgridfs_utime" << endl;
	return -ENOTSUP;
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
	trace() << "-> entering mgridfs_open{file: " << file << "}" << endl;
	trace() << "<- leaving mgridfs_open" << endl;
	return -ENOTSUP;
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
	trace() << "-> entering mgridfs_read{file: " << file << ", len: " << len << ", offset: " << offset << "}" << endl;
	trace() << "<- leaving mgridfs_read" << endl;
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
	trace() << "-> entering mgridfs_write{file: " << file << ", len: " << len << ", offset: " << offset << "}" << endl;
	trace() << "<- leaving mgridfs_write" << endl;
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
	trace() << "-> entering mgridfs_flush{file: " << file << "}" << endl;
	trace() << "<- leaving mgridfs_flush" << endl;
	return -ENOTSUP;
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
	trace() << "-> entering mgridfs_release{file: " << file << "}" << endl;
	trace() << "<- leaving mgridfs_release" << endl;
	return -ENOTSUP;
}

/** Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 *
 * Changed in version 2.2
 */
int mgridfs::mgridfs_fsync(const char *file, int param, struct fuse_file_info *ffinfo) {
	trace() << "-> entering mgridfs_fsync{file: " << file << ", param: " << param << "}" << endl;
	trace() << "<- leaving mgridfs_fsync" << endl;
	return -ENOTSUP;
}

/** Set extended attributes */
int mgridfs::mgridfs_setxattr(const char *file, const char *name, const char *value, size_t len, int flags) {
	trace() << "-> entering mgridfs_setxattr{file: " << file << ", name: " << name << ", len: " << len << "}" << endl;
	//TODO: change the implementation
	trace() << "<- leaving mgridfs_setxattr" << endl;
	return 0;
}

/** Get extended attributes */
int mgridfs::mgridfs_getxattr(const char *file, const char *name, char *value, size_t len) {
	trace() << "-> entering mgridfs_getxattr{file: " << file << ", name: " << name << ", len: " << len << "}" << endl;
	//TODO: change the implementation
	if (len > 0) {
		value[0] = '\0';
	}
	trace() << "<- leaving mgridfs_getxattr" << endl;
	return 0;
}

/** List extended attributes */
int mgridfs::mgridfs_listxattr(const char *file, char *buffer, size_t len) {
	trace() << "-> entering mgridfs_listxattr{file: " << file << ", buflen: " << len << "}" << endl;
	//TODO: change the implementation
	// for now, do nothing and don't support any additional attributes
	
	buffer[0] = '\0';
	trace() << "<- leaving mgridfs_listxattr" << endl;
	return 0;
}

/** Remove extended attributes */
int mgridfs::mgridfs_removexattr(const char *file, const char *attr) {
	trace() << "-> entering mgridfs_removexattr{file: " << file << ", attr: " << attr << "}" << endl;
	if (globalFSOptions._metadataKeyMap.left.find(attr) == globalFSOptions._metadataKeyMap.left.end()) {
		// It is not one of the core attributes of thr GridFS file and will go under metadata.xattr object
		string xAttr = METADATA_XATTR_PREFIX + attr;
		debug() << "Will remove the extra attribute from the file {attr: " << attr 
				<< ", mappedAttr: " << xAttr << "}" << endl;
		//TODO: implementation for actual deletion here
	}

	// for now, do nothing and don't support any additional attributes
	trace() << "<- leaving mgridfs_removexattr" << endl;
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
int mgridfs::mgridfs_create(const char *file, mode_t mode, struct fuse_file_info *ffinfo) {
	trace() << "-> entering mgridfs_create{file: " << file << ", mode: " << std::oct << mode << "}" << endl;
	trace() << "<- leaving mgridfs_create" << endl;
	return -ENOTSUP;
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
	trace() << "-> entering mgridfs_ftruncate{file: " << file << ", offset: " << offset << "}" << endl;
	trace() << "<- leaving mgridfs_ftruncate" << endl;
	return -ENOTSUP;
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
	trace() << "-> entering mgridfs_lock{file: " << path << ", cmd: " << cmd << "}" << endl;
	trace() << "<- leaving mgridfs_lock" << endl;
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
	trace() << "-> entering mgridfs_utimens{file: " << file << "}" << endl;
	trace() << "<- leaving mgridfs_utimens" << endl;
	return -ENOTSUP;
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
	trace() << "-> entering mgridfs_bmap{file: " << file << ", blocksize: " << blocksize << "}" << endl;
	warn() << "not a block device, bmap() is not supported." << endl;
	trace() << "<- leaving mgridfs_bmap" << endl;
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
	trace() << "-> entering mgridfs_ioctl{file: " << file << ", cmd: " << cmd << "flags: " << flags << "}" << endl;
	trace() << "<- leaving mgridfs_ioctl" << endl;
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
	trace() << "-> entering mgridfs_poll{file: " << file << "}" << endl;
	trace() << "<- leaving mgridfs_poll" << endl;
	return -ENOTSUP;
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
	trace() << "-> entering mgridfs_write_buf{file: " << file << ", offset: " << off << "}" << endl;
	trace() << "<- leaving mgridfs_write_buf" << endl;
	return -ENOTSUP;
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
	trace() << "-> entering mgridfs_read_buf{file: " << file << ", size: " << size << ", offset: " << offset << "}" << endl;
	trace() << "<- leaving mgridfs_read_buf" << endl;
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
	trace() << "-> entering mgridfs_flock{file: " << file << ", op: " << op << "}" << endl;
	trace() << "<- leaving mgridfs_flock" << endl;
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
	trace() << "-> entering mgridfs_fallocate{file: " << file << ", mode: " << std::oct << mode << ", offset: " << offset 
			<< ", len: " << len << "}" << endl;
	trace() << "<- leaving mgridfs_fallocate" << endl;
	return -ENOTSUP;
}
