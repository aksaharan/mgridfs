#ifndef mgridfs_file_handle_h
#define mgridfs_file_handle_h

#include <string>
#include <stack>

#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>

using namespace std;

namespace mgridfs {

class FileHandle {
public:
	// Path is ignored in case fh != 0 and there is a mapping for the file handle in the map
	//
	// For fh != 0:  _filename is set with the file assigned for the specified file handle. 
	// For fh = 0: _filename is set with specified valid path. There is no other way of setting the path
	//
	FileHandle(const string& path = "", uint64_t fh = 0);
	~FileHandle();

	bool isValid() const;

	uint64_t assignHandle();
	bool unassignHandle();

	inline uint64_t getHandle() const {
		return _fh;
	}

	const string& getFilename() const {
		return _filename;
	}

	static bool unassignAllHandles(const string& filename);

private:
	// Since same filenames can have multiple file handles but same file handle cannot hanve multiple file
	// association, the relation is as follows for the container:
	// 		Set of file handles vs multiset of file names
	typedef boost::bimap<uint64_t, boost::bimaps::multiset_of<string> > FileHandleMap;

	static FileHandleMap _fileHandles;

	// Cache of recetly freed-up handles. This is useful specially in case of a sparse free handles so that assign does
	// not need to go through cycle of used-up handles to find the next free handle. The worst case for getting a new handle
	// should only be in case the handle space is sparse and there are no handles on the free list.
	// TODO: make use of this structure
	static stack<uint64_t> _freeHandles;

	static uint64_t generateNextHandle(const string& filename);

	FileHandle() {}

	string _filename;
	uint64_t _fh;
};

}

#endif
