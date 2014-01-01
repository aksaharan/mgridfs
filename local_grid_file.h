#ifndef mgridfs_local_grid_file_h
#define mgridfs_local_grid_file_h

#include <fuse.h>

#include <boost/utility.hpp>

namespace mgridfs {

class LocalGridFile : protected boost::noncopyable {
public:
	LocalGridFile();
	virtual ~LocalGridFile();

	virtual size_t getFileSize() const = 0;

	virtual inline uint64_t getFH() const {
		return _fh;
	}

	virtual inline bool isRedaOnly() const {
		return _readOnly;
	}

private:
	bool _readOnly;
	uint64_t _fh;
};

/**
 * LocalGridFile represented as in-memory file until it is saved to the disk
 * It may also contain special logic to move itself from being in-memory to
 * temporary disk based file
 */
class LocalMemoryGridFile : protected LocalGridFile {
public:
	LocalMemoryGridFile();
	~LocalMemoryGridFile();

private:
};

}

#endif
