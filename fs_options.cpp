#include "fs_options.h"
#include "fs_logger.h"
#include "fs_meta_ops.h"

#include <iostream>
#include <mongo/client/connpool.h>

using namespace mgridfs;

namespace { // Anonymous namespace

/*
 * Temporary storage for parsing the arguments using fuse api
 */
struct _ParsedFuseOptions {
	const char* _host;
	const char* _db;
	const char* _collPrefix;
	unsigned int _port;

	const char* _logFile;

	bool _debugEnabled;
	bool _sslEnabled;
};

static struct _ParsedFuseOptions _parsedFuseOptions;

#define MGRIDFS_OPT_KEY(t, p, v) { t, offsetof(struct _ParsedFuseOptions, p), v }

enum MGRIDFS_KEYS {
	KEY_NONE,
	KEY_DEBUG,
	KEY_SSL,
	KEY_HELP,
	KEY_VERSION,
};

struct fuse_opt mgridfsOptions[] = {
	MGRIDFS_OPT_KEY("--host=%s", _host, 0),
	MGRIDFS_OPT_KEY("--port=%d", _port, 0),
	MGRIDFS_OPT_KEY("--db=%s", _db, 0),
	MGRIDFS_OPT_KEY("--collprefix=%s", _collPrefix, 0),
	MGRIDFS_OPT_KEY("--logfile=%s", _logFile, 0),
	FUSE_OPT_KEY("--debug", KEY_DEBUG),
	FUSE_OPT_KEY("--ssl", KEY_SSL),
	FUSE_OPT_KEY("--help", KEY_HELP),
	FUSE_OPT_KEY("--version", KEY_VERSION),
	{NULL}
};

void printHelp(int exitCode) {
	std::cout << "usage: ./mgridfs [options] mountpoint" << std::endl;

	//TODO: Print correct help document
	if (exitCode) {
		::exit(exitCode);
	}
}

int fuseOptionCallback(void* data, const char* arg, int key, struct fuse_args* outArgs) {
	if (key == KEY_HELP) {
		printHelp(0);
		return -1;
	}

	if (key == KEY_VERSION) {
		std::cout << "mgridfs version 0.0.1" << std::endl;
		return -1;
	}

	if (key == KEY_SSL) {
		_parsedFuseOptions._sslEnabled = true;
		std::cout << "Enabled SSL mode support" << std::endl;
		return 0;
	}

	if (key == KEY_DEBUG) {
		_parsedFuseOptions._debugEnabled = true;
		std::cout << "Enabled debug mode" << std::endl;
		return 0;
	}

	return 1;
}

} // End of Anonymous space


bool mgridfs::FSOptions::fromCommandLine(struct fuse_args& fuseArgs) {
	if (fuse_opt_parse(&fuseArgs, &_parsedFuseOptions, mgridfsOptions, fuseOptionCallback) == -1) {
		return false;
	}

	std::cout << "Filesystem invocation requested for following parameters" << std::endl
			<< "  Host: " << _parsedFuseOptions._host << std::endl
			<< "  Port: " << _parsedFuseOptions._port << std::endl
			<< "  DB: " << _parsedFuseOptions._db << std::endl
			<< "  Collection Prefix: " << _parsedFuseOptions._collPrefix << std::endl
			<< "  LogFile: " << _parsedFuseOptions._logFile << std::endl
			<< "  debugEnabled: " << _parsedFuseOptions._debugEnabled << std::endl
			<< "  sslEnabled: " << _parsedFuseOptions._sslEnabled << std::endl
		;

	if (_parsedFuseOptions._logFile) {
		FSLogFile* fsLogFile = new FSLogFile(_parsedFuseOptions._logFile);
		if (fsLogFile) {
			info() << "Initializing file logger {file: " << _parsedFuseOptions._logFile << "}" << endl;
			FSLogManager::get().registerDestination(fsLogFile);
			info() << "Initialized file logger {file: " << _parsedFuseOptions._logFile << "}" << endl;
		} else {
			fatal() << "Failed to initialize log file {file: " << _parsedFuseOptions._logFile << "}, will continue "
					<< "with existing logging facility." << endl;
		}
	}

	if (!_parsedFuseOptions._host) {
		_parsedFuseOptions._host = "localhost";
		std::cout << "Setting mongodb connection hostname -> " << _parsedFuseOptions._host << std::endl;
	}

	if (!_parsedFuseOptions._port) {
		_parsedFuseOptions._port = 27017;
		std::cout << "Setting mongodb connection port -> " << _parsedFuseOptions._port << std::endl;
	} else if (_parsedFuseOptions._port < 0 || _parsedFuseOptions._port > 65535) {
		std::cerr << "Port number cannot be outside valid range [1..65535]: found to be " << _parsedFuseOptions._port;
		return false;
	}

	if (!_parsedFuseOptions._db) {
		_parsedFuseOptions._db = "test";
		std::cout << "Setting mongodb database -> " << _parsedFuseOptions._db << std::endl;
	}

	if (!_parsedFuseOptions._collPrefix) {
		_parsedFuseOptions._collPrefix = "fs";
		std::cout << "Setting mongodb collprefix -> " << _parsedFuseOptions._collPrefix << std::endl;
	}

	globalFSOptions._db = _parsedFuseOptions._db;
	globalFSOptions._collPrefix = _parsedFuseOptions._collPrefix;
	globalFSOptions._host = _parsedFuseOptions._host;
	globalFSOptions._port = _parsedFuseOptions._port;
	globalFSOptions._logFile = _parsedFuseOptions._logFile;
	globalFSOptions._filesNS = _parsedFuseOptions._db + string(".") + _parsedFuseOptions._collPrefix + string(".files");
	globalFSOptions._chunksNS = _parsedFuseOptions._db + string(".") + _parsedFuseOptions._collPrefix + string(".chunks");
	std::cout << "Collection namespaces {Files: " << globalFSOptions._filesNS
			<< ", Chunks: " << globalFSOptions._chunksNS << "}"
			<< std::endl;

	globalFSOptions._hostAndPort = mongo::HostAndPort(_parsedFuseOptions._host, _parsedFuseOptions._port);
	if (!mgridfs::mgridfs_load_or_create_root()) {
		return false;
	}

	// Add metadata bimap for the extended attributes for the files
	globalFSOptions._metadataKeyMap.insert(boost::bimap<string, string>::value_type("file.chunksize", "chunkSize"));
	globalFSOptions._metadataKeyMap.insert(boost::bimap<string, string>::value_type("file.contenttype", "contentType"));
	globalFSOptions._metadataKeyMap.insert(boost::bimap<string, string>::value_type("file.uploaddate", "uploadDate"));
	globalFSOptions._metadataKeyMap.insert(boost::bimap<string, string>::value_type("file.md5", "MD5"));
	globalFSOptions._metadataKeyMap.insert(boost::bimap<string, string>::value_type("file.chunks", "numChunks"));

	return true;
}
