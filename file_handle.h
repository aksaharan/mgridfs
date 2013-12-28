#ifndef mgridfs_file_handle_h
#define mgridfs_file_handle_h

#include <string>
#include <boost/bimap.hpp>

using namespace std;

namespace mgridfs {

class FileHandle{
public:
	FileHandle(const string& path);
	~FileHandle();

	bool isValid() const;
	uint64_t assignHandle();
	bool unassignHandle();

	uint64_t getHandle() const;

private:
	typedef boost::bimap<uint64_t, string> FileHandleMap;

	static FileHandleMap _fileHandles;
	static uint64_t generateNextHandle(const string& filename);

	FileHandle() {}

	string _filename;
};

}

#endif
