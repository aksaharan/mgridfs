#ifndef mgridfs_fs_options_h
#define mgridfs_fs_options_h

#include <fuse.h>

#include <string>
#include <boost/bimap.hpp>

#include <mongo/util/net/hostandport.h>

using namespace std;

namespace mgridfs {

struct FSOptions {
	FSOptions() 
		: _port(0), _debugEnabled(false), _sslEnabled(false), _hostAndPort() {
		// Nothing to do here
	}

	static bool fromCommandLine(struct fuse_args& fuseArgs);

	string _host;
	string _db;
	string _collPrefix;
	unsigned int _port;

	string _logFile;

	bool _debugEnabled;
	bool _sslEnabled;

	mongo::HostAndPort _hostAndPort;
	std::string _filesNS;
	std::string _chunksNS;

	boost::bimap<string, string> _metadataKeyMap;
};

extern FSOptions globalFSOptions;

}

#endif
