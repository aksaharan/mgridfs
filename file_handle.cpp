#include "file_handle.h"
#include "fs_logger.h"

using namespace std;

namespace {
	uint64_t _MIN_FILE_HANDLE = 10;
	uint64_t _FILE_HANDLE = 10;
}

mgridfs::FileHandle::FileHandleMap mgridfs::FileHandle::_fileHandles;

mgridfs::FileHandle::FileHandle(const string& path, uint64_t fh)
	: _filename(path), _fh(fh) {

	if (_fh) {
		FileHandleMap::left_map::const_iterator pIt = _fileHandles.left.find(_fh);
		if (pIt == _fileHandles.left.end()) {
			// Failed to find file for the corresponding file handle in the list of open files handles
			_filename = "";
		} else {
			_filename = pIt->second;
		}
	}
}

mgridfs::FileHandle::~FileHandle() {
	// Nothing to be done here
}

bool mgridfs::FileHandle::isValid() const {
	return (_fileHandles.left.find(_fh) != _fileHandles.left.end());
}

uint64_t mgridfs::FileHandle::assignHandle() {
	if (_filename.empty()) {
		warn() << "Encountered FileHandle::assignHandle for empty filename {filename: " << _filename << "}" << endl;
		return 0;
	}

	// There is no way for the function to know if duplicate assign is being made to generate a file
	// handle. It is caller's flow responsibility to call assign / unassign in the correct
	// order functionally and to manage the associated resource (file handle is one of system resource)
	// correctly.
	_fh = generateNextHandle(_filename);
	if (_fh) {
		_fileHandles.insert(FileHandleMap::value_type(_fh, _filename));
		debug() << "Active file handle tracking {op: assignHandle, count: " << _fileHandles.size() << "}" << endl;
	}
	return _fh;
}

bool mgridfs::FileHandle::unassignHandle() {
	if (_fh) {
		_fileHandles.erase(FileHandleMap::value_type(_fh, _filename));
	}
	debug() << "Active file handle tracking {op: unassignHandle, count: " << _fileHandles.size() << "}" << endl;
	return true;
}

bool mgridfs::FileHandle::unassignAllHandles(const string& filename) {
	vector<uint64_t> fhList;

	// Since bimaps do not support erase by the iterator:
	// 	1. Gather all the file handles for this file name
	// 	2. Release all the handles by itertaing through list from step 1
	for (FileHandleMap::right_map::const_iterator pIt = _fileHandles.right.find(filename);
			pIt != _fileHandles.right.end();
			++pIt) {
		fhList.push_back(pIt->second);
	}

	debug() << "unsassignAllHandles {file: " << filename << ", foundToUnassign: " << fhList.size() << "}" << endl;
	for (vector<uint64_t>::const_iterator pIt = fhList.begin(); pIt != fhList.end(); ++pIt) {
		_fileHandles.erase(FileHandleMap::value_type(*pIt, filename));
	}

	return true;
}

uint64_t mgridfs::FileHandle::generateNextHandle(const string& filename) {
	uint64_t origHandle = _FILE_HANDLE++;
	for (; origHandle != _FILE_HANDLE; ++_FILE_HANDLE) {
		if (_FILE_HANDLE >= 0 && _FILE_HANDLE <= _MIN_FILE_HANDLE) {
			_FILE_HANDLE = _MIN_FILE_HANDLE;
		}

		if (_fileHandles.left.find(_FILE_HANDLE) == _fileHandles.left.end()) {
			break;
		}
	}

	if (origHandle == _FILE_HANDLE) {
		fatal() << "Ran out of file handles {lookupstart: " << origHandle << ", activecount: " << _fileHandles.size() << "}" << endl;
		return 0;
	}

	return _FILE_HANDLE;
}
