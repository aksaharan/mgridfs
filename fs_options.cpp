#include "fs_options.h"
#include "fs_logger.h"
#include "fs_meta_ops.h"
#include "utils.h"

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

	char* _logFile;
	char* _logLevel;
};

static struct _ParsedFuseOptions _parsedFuseOptions;

#define MGRIDFS_OPT_KEY(t, p, v) { t, offsetof(struct _ParsedFuseOptions, p), v }

enum MGRIDFS_KEYS {
	KEY_NONE,
	KEY_HELP,
	KEY_VERSION,
};

struct fuse_opt mgridfsOptions[] = {
	MGRIDFS_OPT_KEY("--host=%s", _host, 0),
	MGRIDFS_OPT_KEY("--port=%d", _port, 0),
	MGRIDFS_OPT_KEY("--db=%s", _db, 0),
	MGRIDFS_OPT_KEY("--collprefix=%s", _collPrefix, 0),
	MGRIDFS_OPT_KEY("--logfile=%s", _logFile, 0),
	MGRIDFS_OPT_KEY("--loglevel=%s", _logLevel, 0),
	FUSE_OPT_KEY("--help", KEY_HELP),
	FUSE_OPT_KEY("--version", KEY_VERSION),
	{NULL}
};

void printHelp(int exitCode) {
	std::cout << "usage: [sudo] ./mgridfs [mgridfs-options] [fuse-options] mountpoint" << endl
			<< " mountpoint              path where mgridfs should be mounted" << endl
			<< endl
			<< "mgridfs-options" << endl
			<< "----------------" << endl
			<< " --host=<host>           mongodb hostname (defaults to localhost)" << endl
			<< " --port=<port>           mongodb port number (defaults to 27017)" << endl
			<< " --db=<db>               mongo GridFS db name" << endl
			<< " --collprefix=<host>     mongo GridFS collection name prefix (defaults to fs)" << endl
			<< " --logfile=<file>        logfile for persistent logging from daemon" << endl
			<< " --loglevel=<level>      logging level for logs" << endl
			<< " --help                  diplay help for command options" << endl
			<< " --version               display mgridfs version information" << endl
			<< endl 
			<< "fuse-options (\"man mount.fuse\" for detailed options)" << endl
			<< "-------------------------------------------------------" << endl
		;
		
	//TODO: Print correct help document
	::exit(exitCode);
}

int fuseOptionCallback(void* data, const char* arg, int key, struct fuse_args* outArgs) {
	if (key == KEY_HELP) {
		printHelp(0);
		return -1;
	}

	if (key == KEY_VERSION) {
		std::cout << "mgridfs " << mgridfs::MGRIDFS_MAJOR_VERSION
				<< "." << mgridfs::MGRIDFS_MINOR_VERSION
				<< "." << mgridfs::MGRIDFS_PATCH_VERSION
				<< endl
			;
		::exit(0);
	}

	return 1;
}

} // End of Anonymous space


// Define version number
const unsigned int mgridfs::MGRIDFS_MAJOR_VERSION = 0;
const unsigned int mgridfs::MGRIDFS_MINOR_VERSION = 1;
const unsigned int mgridfs::MGRIDFS_PATCH_VERSION = 0;


bool mgridfs::FSOptions::fromCommandLine(struct fuse_args& fuseArgs) {
	if (fuse_opt_parse(&fuseArgs, &_parsedFuseOptions, mgridfsOptions, fuseOptionCallback) == -1) {
		return false;
	}

	info() << "Filesystem invocation requested for following parameters" << endl
			<< "  Host: " << _parsedFuseOptions._host << endl
			<< "  Port: " << _parsedFuseOptions._port << endl
			<< "  DB: " << _parsedFuseOptions._db << endl
			<< "  Collection Prefix: " << _parsedFuseOptions._collPrefix << endl
			<< "  LogFile: " << _parsedFuseOptions._logFile << endl
			<< "  LogLevel: " << _parsedFuseOptions._logLevel << endl
		;

	if (!_parsedFuseOptions._host) {
		_parsedFuseOptions._host = "localhost";
		info() << "Setting mongodb connection hostname -> " << _parsedFuseOptions._host << endl;
	}

	if (!_parsedFuseOptions._port) {
		_parsedFuseOptions._port = 27017;
		info() << "Setting mongodb connection port -> " << _parsedFuseOptions._port << endl;
	} else if (_parsedFuseOptions._port < 0 || _parsedFuseOptions._port > 65535) {
		fatal() << "Port number cannot be outside valid range [1..65535]: found to be " << _parsedFuseOptions._port;
		return false;
	}

	if (!_parsedFuseOptions._db) {
		_parsedFuseOptions._db = "test";
		info() << "Setting mongodb database -> " << _parsedFuseOptions._db << endl;
	}

	if (!_parsedFuseOptions._collPrefix) {
		_parsedFuseOptions._collPrefix = "fs";
		info() << "Setting mongodb collprefix -> " << _parsedFuseOptions._collPrefix << endl;
	}

	globalFSOptions._db = _parsedFuseOptions._db;
	globalFSOptions._collPrefix = _parsedFuseOptions._collPrefix;
	globalFSOptions._host = _parsedFuseOptions._host;
	globalFSOptions._port = _parsedFuseOptions._port;
	globalFSOptions._logFile = _parsedFuseOptions._logFile;
	if (_parsedFuseOptions._logLevel) {
		globalFSOptions._logLevel = FSLogManager::get().stringToLogLevel(toUpper(_parsedFuseOptions._logLevel));
		if (globalFSOptions._logLevel == LL_INVALID) {
			error() << "Invalid logLevel specified on the command-line, will default to INFO level" << endl;
			globalFSOptions._logLevel = LL_INFO;
		}
	} else {
		globalFSOptions._logLevel = LL_INFO;
	}
	info() << "Setting log level {LogLevel: " << FSLogManager::get().logLevelToString(globalFSOptions._logLevel) << "}" << endl;
	FSLogManager::get().setLogLevel(globalFSOptions._logLevel);

	globalFSOptions._filesNS = _parsedFuseOptions._db + string(".") + _parsedFuseOptions._collPrefix + string(".files");
	globalFSOptions._chunksNS = _parsedFuseOptions._db + string(".") + _parsedFuseOptions._collPrefix + string(".chunks");
	info() << "Collection namespaces {Files: " << globalFSOptions._filesNS
			<< ", Chunks: " << globalFSOptions._chunksNS << "}"
			<< endl;

	globalFSOptions._hostAndPort = mongo::HostAndPort(_parsedFuseOptions._host, _parsedFuseOptions._port);
	if (!mgridfs::mgridfs_load_or_create_root()) {
		return false;
	}

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

	// Add metadata bimap for the extended attributes for the files
	globalFSOptions._metadataKeyMap.insert(boost::bimap<string, string>::value_type("file.chunksize", "chunkSize"));
	globalFSOptions._metadataKeyMap.insert(boost::bimap<string, string>::value_type("file.contenttype", "contentType"));
	globalFSOptions._metadataKeyMap.insert(boost::bimap<string, string>::value_type("file.uploaddate", "uploadDate"));
	globalFSOptions._metadataKeyMap.insert(boost::bimap<string, string>::value_type("file.md5", "MD5"));
	globalFSOptions._metadataKeyMap.insert(boost::bimap<string, string>::value_type("file.chunks", "numChunks"));

	return true;
}
