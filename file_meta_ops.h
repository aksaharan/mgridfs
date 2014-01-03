#ifndef mgridfs_file_meta_ops_h
#define mgridfs_file_meta_ops_h

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fuse.h>


namespace mgridfs {

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.	 The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int mgridfs_getattr(const char*, struct stat*);

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
int mgridfs_fgetattr(const char *, struct stat *, struct fuse_file_info *);

/** Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.	If the linkname is too long to fit in the
 * buffer, it should be truncated.	The return value should be 0
 * for success.
 */
int mgridfs_readlink(const char *, char *, size_t);

/** Create a file node
 *
 * This is called for creation of all non-directory, non-symlink
 * nodes.  If the filesystem defines a create() method, then for
 * regular files that will be called instead.
 */
int mgridfs_mknod(const char *, mode_t, dev_t);

/** Remove a file */
int mgridfs_unlink(const char *);

/** Create a symbolic link */
int mgridfs_symlink(const char *, const char *);

/** Rename a file */
int mgridfs_rename(const char *, const char *);

/** Create a hard link to a file */
int mgridfs_link(const char *, const char *);

/** Change the permission bits of a file */
int mgridfs_chmod(const char *, mode_t);

/** Change the owner and group of a file */
int mgridfs_chown(const char *, uid_t, gid_t);

/** Change the size of a file */
int mgridfs_truncate(const char *, off_t);

/** Change the access and/or modification times of a file
 *
 * Deprecated, use utimens() instead.
 */
int mgridfs_utime(const char *, struct utimbuf *);

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
int mgridfs_open(const char *, struct fuse_file_info *);

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
int mgridfs_read(const char *, char *, size_t, off_t, struct fuse_file_info *);

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
int mgridfs_read_buf(const char *, struct fuse_bufvec **bufp, size_t size, off_t off, struct fuse_file_info *);

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.	 An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
int mgridfs_write(const char *, const char *, size_t, off_t, struct fuse_file_info *);

/** Write contents of buffer to an open file
 *
 * Similar to the write() method, but data is supplied in a
 * generic buffer.  Use fuse_buf_copy() to transfer data to
 * the destination.
 *
 * Introduced in version 2.9
 */

int mgridfs_write_buf(const char *, struct fuse_bufvec *buf, off_t off, struct fuse_file_info *);

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
int mgridfs_flush(const char *, struct fuse_file_info *);

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
int mgridfs_release(const char *, struct fuse_file_info *);

/** Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 *
 * Changed in version 2.2
 */
int mgridfs_fsync(const char *, int, struct fuse_file_info *);

/** Set extended attributes */
int mgridfs_setxattr(const char *, const char *, const char *, size_t, int);

/** Get extended attributes */
int mgridfs_getxattr(const char *, const char *, char *, size_t);

/** List extended attributes */
int mgridfs_listxattr(const char *, char *, size_t);

/** Remove extended attributes */
int mgridfs_removexattr(const char *, const char *);

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
int mgridfs_create(const char *, mode_t, struct fuse_file_info *);

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
int mgridfs_ftruncate(const char *, off_t, struct fuse_file_info *);

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
int mgridfs_lock(const char *, struct fuse_file_info *, int cmd, struct flock *);

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
int mgridfs_utimens(const char *, const struct timespec tv[2]);

/**
 * Map block index within file to block index within device
 *
 * Note: This makes sense only for block device backed filesystems
 * mounted with the 'blkdev' option
 *
 * Introduced in version 2.6
 */
int mgridfs_bmap(const char *, size_t blocksize, uint64_t *idx);

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
int mgridfs_ioctl(const char *, int cmd, void *arg, struct fuse_file_info *, unsigned int flags, void *data);

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
int mgridfs_poll(const char *, struct fuse_file_info *, struct fuse_pollhandle *ph, unsigned *reventsp);

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
int mgridfs_flock(const char *, struct fuse_file_info *, int op);

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
int mgridfs_fallocate(const char *, int, off_t, off_t, struct fuse_file_info *);

}

#endif
