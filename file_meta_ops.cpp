#include "file_meta_ops.h"
#include "fs_logger.h"

#include <string.h>
#include <errno.h>

#include <iostream>


/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.	 The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int mgridfs::mgridfs_getattr(const char* file, struct stat* file_stat) {
#if 0
	struct stat {
		dev_t     st_dev;     /* ID of device containing file */
		ino_t     st_ino;     /* inode number */
		mode_t    st_mode;    /* protection */
		nlink_t   st_nlink;   /* number of hard links */
		uid_t     st_uid;     /* user ID of owner */
		gid_t     st_gid;     /* group ID of owner */
		dev_t     st_rdev;    /* device ID (if special file) */
		off_t     st_size;    /* total size, in bytes */
		blksize_t st_blksize; /* blocksize for file system I/O */
		blkcnt_t  st_blocks;  /* number of 512B blocks allocated */
		time_t    st_atime;   /* time of last access */
		time_t    st_mtime;   /* time of last modification */
		time_t    st_ctime;   /* time of last status change */
	};
#endif

	trace() << "-> entering mgridfs_getattr(" << file << ")" << endl;
	bzero(file_stat, sizeof(*file_stat));
	file_stat->st_uid = geteuid();
	file_stat->st_gid = getegid();
	file_stat->st_mode = S_IFDIR | 0777;
	file_stat->st_nlink = 2;
//	file_stat->st_rdev = 0;
//	file_stat->st_ino = 0;
//	file_stat->st_dev = 0;
//	file_stat->st_blksize = 1;
//	file_stat->st_blocks = 1;
//	file_stat->st_size = 10;
	file_stat->st_atime = time(NULL);
	file_stat->st_mtime = time(NULL) - 100;
	file_stat->st_ctime = time(NULL) - 200;

	trace() << "<- leaving mgridfs_getattr(" << file << ")" << endl;
	return 0;
}

/** Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.	If the linkname is too long to fit in the
 * buffer, it should be truncated.	The return value should be 0
 * for success.
 */
int mgridfs::mgridfs_readlink(const char *, char *, size_t) {
	trace() << "-> entering mgridfs_readlink()" << endl;
	trace() << "<- leaving mgridfs_readlink()" << endl;
	return ENOTSUP;
}

/** Create a file node
 *
 * This is called for creation of all non-directory, non-symlink
 * nodes.  If the filesystem defines a create() method, then for
 * regular files that will be called instead.
 */
int mgridfs::mgridfs_mknod(const char *, mode_t, dev_t) {
	trace() << "-> entering mgridfs_mknod()" << endl;
	trace() << "<- leaving mgridfs_mknod()" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Remove a file */
int mgridfs::mgridfs_unlink(const char *) {
	trace() << "-> entering mgridfs_unlink()" << endl;
	trace() << "<- leaving mgridfs_unlink()" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Create a symbolic link */
int mgridfs::mgridfs_symlink(const char *, const char *) {
	trace() << "-> entering mgridfs_symlink()" << endl;
	trace() << "<- leaving mgridfs_symlink()" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Rename a file */
int mgridfs::mgridfs_rename(const char *, const char *) {
	trace() << "-> entering mgridfs_rename()" << endl;
	trace() << "<- leaving mgridfs_rename()" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Create a hard link to a file */
int mgridfs::mgridfs_link(const char *, const char *) {
	trace() << "-> entering mgridfs_link()" << endl;
	trace() << "<- leaving mgridfs_link()" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Change the permission bits of a file */
int mgridfs::mgridfs_chmod(const char *, mode_t) {
	trace() << "-> entering mgridfs_chmod()" << endl;
	trace() << "<- leaving mgridfs_chmod()" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Change the owner and group of a file */
int mgridfs::mgridfs_chown(const char *, uid_t, gid_t) {
	trace() << "-> entering mgridfs_chown()" << endl;
	trace() << "<- leaving mgridfs_chown()" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Change the size of a file */
int mgridfs::mgridfs_truncate(const char *, off_t) {
	trace() << "-> entering mgridfs_truncate()" << endl;
	trace() << "<- leaving mgridfs_truncate()" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Change the access and/or modification times of a file
 *
 * Deprecated, use utimens() instead.
 */
int mgridfs::mgridfs_utime(const char *, struct utimbuf *) {
	trace() << "-> entering mgridfs_utime()" << endl;
	trace() << "<- leaving mgridfs_utime()" << endl;
	errno = ENOTSUP;
	return -1;
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
int mgridfs::mgridfs_open(const char *, struct fuse_file_info *) {
	trace() << "-> entering mgridfs_open()" << endl;
	trace() << "<- leaving mgridfs_open()" << endl;
	errno = ENOTSUP;
	return -1;
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
int mgridfs::mgridfs_read(const char *, char *, size_t, off_t, struct fuse_file_info *) {
	trace() << "-> entering mgridfs_read()" << endl;
	trace() << "<- leaving mgridfs_read()" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.	 An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
int mgridfs::mgridfs_write(const char *, const char *, size_t, off_t, struct fuse_file_info *) {
	trace() << "-> entering mgridfs_write()" << endl;
	trace() << "<- leaving mgridfs_write()" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Get file system statistics
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 *
 * Replaced 'struct statfs' parameter with 'struct statvfs' in
 * version 2.5
 */
int mgridfs::mgridfs_statfs(const char *, struct statvfs *) {
	trace() << "-> entering mgridfs_statfs()" << endl;
	trace() << "<- leaving mgridfs_statfs()" << endl;
	errno = ENOTSUP;
	return -1;
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
int mgridfs::mgridfs_flush(const char *, struct fuse_file_info *) {
	trace() << "-> entering mgridfs_flush()" << endl;
	trace() << "<- leaving mgridfs_flush()" << endl;
	errno = ENOTSUP;
	return -1;
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
int mgridfs::mgridfs_release(const char *, struct fuse_file_info *) {
	trace() << "-> entering mgridfs_release()" << endl;
	trace() << "<- leaving mgridfs_release()" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 *
 * Changed in version 2.2
 */
int mgridfs::mgridfs_fsync(const char *, int, struct fuse_file_info *) {
	trace() << "-> entering mgridfs_fsync()" << endl;
	trace() << "<- leaving mgridfs_fsync()" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Set extended attributes */
int mgridfs::mgridfs_setxattr(const char *, const char *, const char *, size_t, int) {
	trace() << "-> entering mgridfs_setxattr()" << endl;
	trace() << "<- leaving mgridfs_setxattr()" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Get extended attributes */
int mgridfs::mgridfs_getxattr(const char *, const char *, char *, size_t) {
	trace() << "-> entering mgridfs_getxattr()" << endl;
	trace() << "<- leaving mgridfs_getxattr()" << endl;
	errno = ENOTSUP;
	return -1;
}

/** List extended attributes */
int mgridfs::mgridfs_listxattr(const char *, char *, size_t) {
	trace() << "-> entering mgridfs_listxattr()" << endl;
	trace() << "<- leaving mgridfs_listxattr()" << endl;
	errno = ENOTSUP;
	return -1;
}

/** Remove extended attributes */
int mgridfs::mgridfs_removexattr(const char *, const char *) {
	trace() << "-> entering mgridfs_removexattr()" << endl;
	trace() << "<- leaving mgridfs_removexattr()" << endl;
	errno = ENOTSUP;
	return -1;
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
int mgridfs::mgridfs_create(const char *, mode_t, struct fuse_file_info *) {
	trace() << "-> entering mgridfs_create()" << endl;
	trace() << "<- leaving mgridfs_create()" << endl;
	errno = ENOTSUP;
	return -1;
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
int mgridfs::mgridfs_ftruncate(const char *, off_t, struct fuse_file_info *) {
	trace() << "-> entering mgridfs_ftruncate()" << endl;
	trace() << "<- leaving mgridfs_ftruncate()" << endl;
	errno = ENOTSUP;
	return -1;
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
int mgridfs::mgridfs_fgetattr(const char *, struct stat *, struct fuse_file_info *) {
	trace() << "-> entering mgridfs_fgetattr()" << endl;
	trace() << "<- leaving mgridfs_fgetattr()" << endl;
	errno = ENOTSUP;
	return -1;
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
int mgridfs::mgridfs_lock(const char *, struct fuse_file_info *, int cmd, struct flock *) {
	trace() << "-> entering mgridfs_lock()" << endl;
	trace() << "<- leaving mgridfs_lock()" << endl;
	errno = ENOTSUP;
	return -1;
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
int mgridfs::mgridfs_access(const char *, int) {
	trace() << "-> entering mgridfs_access()" << endl;
	trace() << "<- leaving mgridfs_access()" << endl;
	errno = ENOTSUP;
	return -1;
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
int mgridfs::mgridfs_utimens(const char *, const struct timespec tv[2]) {
	trace() << "-> entering mgridfs_utimens()" << endl;
	trace() << "<- leaving mgridfs_utimens()" << endl;
	return ENOTSUP;
}

/**
 * Map block index within file to block index within device
 *
 * Note: This makes sense only for block device backed filesystems
 * mounted with the 'blkdev' option
 *
 * Introduced in version 2.6
 */
int mgridfs::mgridfs_bmap(const char *, size_t blocksize, uint64_t *idx) {
	trace() << "-> entering mgridfs_bmap()" << endl;
	trace() << "<- leaving mgridfs_bmap()" << endl;
	return ENOTSUP;
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
int mgridfs::mgridfs_ioctl(const char *, int cmd, void *arg, struct fuse_file_info *, unsigned int flags, void *data) {
	trace() << "-> entering mgridfs_ioctl()" << endl;
	trace() << "<- leaving mgridfs_ioctl()" << endl;
	return ENOTSUP;
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
int mgridfs::mgridfs_poll(const char *, struct fuse_file_info *, struct fuse_pollhandle *ph, unsigned *reventsp) {
	trace() << "-> entering mgridfs_poll()" << endl;
	trace() << "<- leaving mgridfs_poll()" << endl;
	return ENOTSUP;
}

/** Write contents of buffer to an open file
 *
 * Similar to the write() method, but data is supplied in a
 * generic buffer.  Use fuse_buf_copy() to transfer data to
 * the destination.
 *
 * Introduced in version 2.9
 */
int mgridfs::mgridfs_write_buf(const char *, struct fuse_bufvec *buf, off_t off, struct fuse_file_info *) {
	trace() << "-> entering mgridfs_write_buf()" << endl;
	trace() << "<- leaving mgridfs_write_buf()" << endl;
	return ENOTSUP;
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
int mgridfs::mgridfs_read_buf(const char *, struct fuse_bufvec **bufp, size_t size, off_t off, struct fuse_file_info *) {
	trace() << "-> entering mgridfs_read_buf()" << endl;
	trace() << "<- leaving mgridfs_read_buf()" << endl;
	return ENOTSUP;
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
int mgridfs::mgridfs_flock(const char *, struct fuse_file_info *, int op) {
	trace() << "-> entering mgridfs_flock()" << endl;
	trace() << "<- leaving mgridfs_flock()" << endl;
	return ENOTSUP;
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
int mgridfs::mgridfs_fallocate(const char *, int, off_t, off_t, struct fuse_file_info *) {
	trace() << "-> entering mgridfs_fallocate()" << endl;
	trace() << "<- leaving mgridfs_fallocate()" << endl;
	return ENOTSUP;
}
