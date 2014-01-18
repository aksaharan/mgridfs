#ifndef mgridfs_fs_options_h
#define mgridfs_fs_options_h

#include "fs_logger.h"

#include <fuse.h>

#include <string>
#include <boost/bimap.hpp>

#include <mongo/util/net/hostandport.h>

using namespace std;

namespace mgridfs {

extern const unsigned int MGRIDFS_MAJOR_VERSION;
extern const unsigned int MGRIDFS_MINOR_VERSION;
extern const unsigned int MGRIDFS_PATCH_VERSION;

struct FSOptions {
	FSOptions() 
		: _port(0), _hostAndPort() {
		// Nothing to do here
	}

	static bool fromCommandLine(struct fuse_args& fuseArgs);

	string _host;
	string _db;
	string _collPrefix;
	string _connectString;
	unsigned int _port;

	string _logFile;
	LogLevel _logLevel;

	mongo::HostAndPort _hostAndPort;
	std::string _filesNS;
	std::string _chunksNS;

	unsigned long _memChunkSize;
	unsigned long _maxMemFileChunks;
	bool _enableDynMemChunk;

	boost::bimap<string, string> _metadataKeyMap;
};

extern FSOptions globalFSOptions;

}

#endif
