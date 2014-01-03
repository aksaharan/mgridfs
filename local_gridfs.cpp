#include "local_gridfs.h"

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

LocalGridFile* LocalGridFS::findByName(const string& file) {
	return NULL;
}

LocalGridFile* LocalGridFS::createFile(const string& filename) {
	return NULL;
}

bool LocalGridFS::releaseFile(const string& filename) {
	return false;
}

bool LocalGridFS::releaseAllFiles(bool flushAll) {
	return false;
}
