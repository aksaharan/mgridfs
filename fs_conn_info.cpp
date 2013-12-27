#include "fs_conn_info.h"
#include "fs_logger.h"

#include <iostream>

mgridfs::FSConnInfo::FSConnInfo() {
	trace() << "FSConnInfo::FSConnInfo()" << endl;
}

mgridfs::FSConnInfo::FSConnInfo(const FSConnInfo&) {
	trace() << "FSConnInfo::FSConnInfo(FSConnInfo&)" << endl;
}

mgridfs::FSConnInfo::~FSConnInfo() {
	trace() << "FSConnInfo::~FSConnInfo()" << endl;
}
