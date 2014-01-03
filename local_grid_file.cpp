#include "local_grid_file.h"

using namespace mgridfs;

namespace {
	const size_t LOCAL_FILE_CHUNK_SIZE = 128 * 1024;
}

LocalGridFile::LocalGridFile()
	: _readOnly(true), _dirty(false), _filename("") {
}

LocalGridFile::LocalGridFile(const string& filename)
	: _readOnly(true), _dirty(false), _filename(filename) {
}

LocalGridFile::~LocalGridFile() {
}

