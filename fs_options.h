#ifndef mgridfs_fs_options_h
#define mgridfs_fs_options_h

#include <fuse.h>

#include <mongo/util/net/hostandport.h>

namespace mgridfs {

struct FSOptions {
	FSOptions() 
		: _host(NULL), _db(NULL), _coll(NULL), _port(0), _logFile(NULL), _debugEnabled(false)
		, _sslEnabled(false) {
		// Nothing to do here
	}

	static bool fromCommandLine(struct fuse_args& fuseArgs);

	const char* _host;
	const char* _db;
	const char* _coll;
	unsigned int _port;

	const char* _logFile;

	bool _debugEnabled;
	bool _sslEnabled;

	mongo::HostAndPort _hostAndPort;
};

extern mgridfs::FSOptions globalFSOptions;

}

#endif
