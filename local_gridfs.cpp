#include "local_gridfs.h"
#include "local_grid_file.h"
#include "fs_logger.h"

using namespace mgridfs;

map<string, LocalGridFile*> mgridfs::LocalGridFS::_localGridFileMap;

LocalGridFS::LocalGridFS() {
}

LocalGridFS::~LocalGridFS() {
}

LocalGridFS& LocalGridFS::get() {
	static LocalGridFS localGridFS;
	return localGridFS;
}

LocalGridFile* LocalGridFS::findByName(const string& filename) {
	LocalGridFileMap::const_iterator pIt = _localGridFileMap.find(filename);
	if (pIt == _localGridFileMap.end()) {
		return NULL;
	}

	return pIt->second;
}

LocalGridFile* LocalGridFS::createFile(const string& filename) {
	// Enhance further to include local on-disk and hybrid approach for managing files more
	// efficiently without high RAM usage on the system
	LocalGridFile* localGridFile = new (nothrow) LocalMemoryGridFile(filename);
	if (localGridFile) {
		// If the local file was allocated, add the specified filename to the map as well
		_localGridFileMap.insert(LocalGridFileMap::value_type(filename, localGridFile));
	}

	return localGridFile;
}

bool LocalGridFS::releaseFile(const string& filename) {
	LocalGridFileMap::const_iterator pIt = _localGridFileMap.find(filename);
	if (pIt == _localGridFileMap.end()) {
		warn() << "File not found for releaseFile {file: " << filename << "}" << endl;
		return true; // Although file is not found, it is not really an error
	}

	LocalGridFile* localGridFile = pIt->second;
	_localGridFileMap.erase(filename);
	if (!localGridFile) {
		error() << "Encountered NULL file entry in releaseFile {file: " << filename << "}" << endl;
		return false;
	}

	if (localGridFile->flush()) {
		warn() << "Failed to flush before deletion in releaseFile {file: " << filename << "}, may result in corrupt file data." 
			<< endl;
	}

	delete localGridFile;
	return true;
}

bool LocalGridFS::releaseAllFiles(bool flushAll) {
	return false;
}
