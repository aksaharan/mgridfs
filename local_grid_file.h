#ifndef mgridfs_local_grid_file_h
#define mgridfs_local_grid_file_h

#include <fuse.h>

#include <vector>
#include <string>
#include <memory>
#include <boost/utility.hpp>
#include <boost/smart_ptr/shared_array.hpp>

using namespace std;

namespace mongo {
	class GridFile;
}

namespace mgridfs {

class LocalGridFile : protected boost::noncopyable {
public:
	LocalGridFile();
	LocalGridFile(const string& filename);
	virtual ~LocalGridFile();

	virtual inline size_t getCapacity() const { return _capacity; }
	virtual inline size_t getSize() const { return _size; }
	virtual inline bool isReadOnly() const { return _readOnly; }
	virtual inline const string& getFilename() const { return _filename; }

	virtual bool setCapacity(size_t size) = 0;
	virtual bool setSize(size_t size) = 0;
	virtual bool setReadOnly() = 0;
	virtual bool setFilename(const string& filename) = 0;
	virtual void setDirty(bool flag) = 0;

	virtual int openRemote(int fileFlags) = 0;
	virtual int write(const char *data, size_t len, off_t offset) = 0;
	virtual int read(char *data, size_t len, off_t offset) const = 0;
	virtual int flush() = 0;

	virtual inline bool isDirty() const { return _dirty; }

protected:
	size_t _size;
	size_t _capacity;
	bool _readOnly;
	bool _dirty;
	string _filename;
};

/**
 * LocalGridFile represented as in-memory file until it is saved to the disk
 * It may also contain special logic to move itself from being in-memory to
 * temporary disk based file
 */
class LocalMemoryGridFile : public LocalGridFile {
public:
	LocalMemoryGridFile();
	LocalMemoryGridFile(const string& filename);
	~LocalMemoryGridFile();

	virtual bool setCapacity(size_t size);
	virtual bool setSize(size_t size);
	virtual bool setReadOnly();
	virtual bool setFilename(const string& filename);
	virtual void setDirty(bool flag);

	virtual int openRemote(int fileFlags);
	virtual int write(const char *data, size_t len, off_t offset);
	virtual int read(char *data, size_t len, off_t offset) const;
	virtual int flush();

protected:
	virtual int _write(const char *data, size_t len, off_t offset);

private:
	size_t _chunkSize;
	vector<char*> _chunks;
	//boost::thread::mutex _fileLock;


	boost::shared_array<char> createFlushBuffer(size_t& bufferLen) const;
	bool initLocalBuffers(mongo::GridFile& gridFile);
};

}

#endif
