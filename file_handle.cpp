#include "file_handle.h"
#include "fs_logger.h"

using namespace std;

namespace {
	uint64_t _MIN_FILE_HANDLE = 10;
	uint64_t _FILE_HANDLE = 10;
}

mgridfs::FileHandle::FileHandleMap mgridfs::FileHandle::_fileHandles;

mgridfs::FileHandle::FileHandle(const string& path)
	: _filename(path) {
}

mgridfs::FileHandle::~FileHandle() {
	// Nothing to be done here
}

bool mgridfs::FileHandle::isValid() const {
	return (_fileHandles.right.find(_filename) != _fileHandles.right.end());
}

uint64_t mgridfs::FileHandle::assignHandle() {
	if (_filename.empty()) {
		warn() << "Encountered FileHandle::assignHandle for empty filename {filename: " << _filename << "}" << endl;
		return 0;
	}

	uint64_t handle = getHandle();
	if (handle) {
		// Handle is already assigned to this file, just return success in this case
		return handle;
	}

	handle = generateNextHandle(_filename);
	if (!handle) {
		return 0;
	}

	_fileHandles.insert(FileHandleMap::value_type(handle, _filename));
	trace() << "Active file handle tracking {op: assignHandle, count: " << _fileHandles.size() << "}" << endl;
	return handle;
}

bool mgridfs::FileHandle::unassignHandle() {
	uint64_t handle = getHandle();
	if (handle) {
		_fileHandles.erase(FileHandleMap::value_type(handle, _filename));
	}
	trace() << "Active file handle tracking {op: unassignHandle, count: " << _fileHandles.size() << "}" << endl;
	return true;
}

uint64_t mgridfs::FileHandle::getHandle() const {
	FileHandleMap::right_map::const_iterator pIt = _fileHandles.right.find(_filename);
	if (pIt == _fileHandles.right.end()) {
		return 0;
	}
	return pIt->second;
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
