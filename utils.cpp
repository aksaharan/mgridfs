#include "utils.h"

#include <libgen.h>
#include <string.h>
#include <stdlib.h>

using namespace mgridfs;

string mgridfs::getPathBasename(const string& path) {
	char* pathTemp = strdup(path.c_str());
	char* baseName = basename(pathTemp);
	if (baseName[0] == '/') {
		baseName[0] = 0;
	}

	string baseNameStr(baseName);
	free(pathTemp);

	return baseNameStr;
}

string mgridfs::getPathDirname(const string& path) {
	char* pathTemp = strdup(path.c_str());
	char* dirName = dirname(pathTemp);

	string dirNameStr(dirName);
	free(pathTemp);

	return dirNameStr;
}

char* mgridfs::toUpper(char* str) {
	char* retStr = str;
	if (str) {
		for ( ; *str; ++str)
			*str = toupper(*str);
	}

	return retStr;
}

unsigned long mgridfs::get512BlockCount(unsigned long size) {
	return ((size + 511) / 512);
}
