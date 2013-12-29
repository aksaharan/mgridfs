#ifndef mgridfs_util_h
#define mgridfs_util_h

#include <string>

using namespace std;

namespace mgridfs {

string getPathBasename(const string& path);
string getPathDirname(const string& path);

char* toUpper(char* str);

unsigned long get512BlockCount(unsigned long size);

}

#endif
