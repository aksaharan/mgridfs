#ifndef mgridfs_local_grid_file_h
#define mgridfs_local_grid_file_h

#include <fuse.h>

#include <string>
#include <boost/utility.hpp>

using namespace std;

namespace mgridfs {

class LocalGridFile : protected boost::noncopyable {
public:
	LocalGridFile();
	LocalGridFile(const string& filename);
	virtual ~LocalGridFile();

	virtual size_t getFileSize() const = 0;
	virtual void setFileSize(size_t size) const = 0;

	virtual int openRemote() = 0;
	virtual int write(const char *data, size_t len, off_t offset) = 0;
	virtual int read(char *data, size_t len, off_t offset) const = 0;
	virtual int flush() = 0;

	virtual inline bool isReadOnly() const { return _readOnly; }
	virtual inline void setReadOnly() { _readOnly = true; }

	virtual inline bool isDirty() const { return _dirty; }
	virtual inline void setDirty() { _dirty = true; }

	virtual inline string getFilename() const { return _filename; }
	virtual inline void setFilename(const string& filename) { _filename = filename; }

private:
	bool _readOnly;
	bool _dirty;
	string _filename;
};

/**
 * LocalGridFile represented as in-memory file until it is saved to the disk
 * It may also contain special logic to move itself from being in-memory to
 * temporary disk based file
 */
class LocalMemoryGridFile : protected LocalGridFile {
public:
	LocalMemoryGridFile();
	LocalMemoryGridFile(const string& filename);
	~LocalMemoryGridFile();

private:
};

}

#endif
