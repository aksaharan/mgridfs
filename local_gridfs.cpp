#include "local_gridfs.h"

using namespace mgridfs;

LocalGridFS& LocalGridFS::get() {
	static LocalGridFS localGridFS;
	return localGridFS;
}

LocalGridFile* LocalGridFS::findByName(const string& file) {
	return NULL;
}

LocalGridFS::LocalGridFS() {
}

LocalGridFS::~LocalGridFS() {
}
