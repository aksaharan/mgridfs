#include "fs_options.h"
#include "fs_logger.h"

#include <iostream>
#include <mongo/client/connpool.h>

using namespace mgridfs;

namespace { // Anonymous namespace

#define MGRIDFS_OPT_KEY(t, p, v) { t, offsetof(struct mgridfs::FSOptions, p), v }

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
	MGRIDFS_OPT_KEY("--coll=%s", _coll, 0),
	MGRIDFS_OPT_KEY("--logfile=%s", _logFile, 0),
	FUSE_OPT_KEY("--debug", KEY_DEBUG),
	FUSE_OPT_KEY("--ssl", KEY_SSL),
	FUSE_OPT_KEY("--help", KEY_HELP),
	FUSE_OPT_KEY("--version", KEY_VERSION),
	{NULL}
};

void printHelp(int exitCode) {
	std::cout << "usage: ./mgridfs [options] mountpoint" << std::endl;
	if (exitCode) {
		::exit(exitCode);
	}
}

int fuseOptionCallback(void* data, const char* arg, int key, struct fuse_args* outArgs) {
	std::cerr << "Called CallBack for -> " << key << ", Data: " << arg << std::endl;
	if (key == KEY_HELP) {
		printHelp(0);
		return -1;
	}

	if (key == KEY_VERSION) {
		std::cout << "mgridfs version 0.0.1" << std::endl;
		return -1;
	}

	if (key == KEY_SSL) {
		mgridfs::globalFSOptions._sslEnabled = true;
		std::cout << "Enabled SSL mode support" << std::endl;
		return 0;
	}

	if (key == KEY_DEBUG) {
		mgridfs::globalFSOptions._debugEnabled = true;
		std::cout << "Enabled debug mode" << std::endl;
		return 0;
	}

	return 1;
}

} // End of Anonymous space


bool mgridfs::FSOptions::fromCommandLine(struct fuse_args& fuseArgs) {
	if (fuse_opt_parse(&fuseArgs, &globalFSOptions, mgridfsOptions, fuseOptionCallback) == -1) {
		return false;
	}

	std::cout << "Filesystem invocation requested for following parameters" << std::endl
			<< "Host: " << globalFSOptions._host << std::endl
			<< "Port: " << globalFSOptions._port << std::endl
			<< "DB: " << globalFSOptions._db << std::endl
			<< "Collection: " << globalFSOptions._coll << std::endl
			<< "LogFile: " << globalFSOptions._logFile << std::endl
			<< "debugEnabled: " << globalFSOptions._debugEnabled << std::endl
			<< "sslEnabled: " << globalFSOptions._sslEnabled << std::endl
		;

	if (globalFSOptions._logFile) {
		if (!FSLogger::initialize(globalFSOptions._logFile)) {
			std::cout << "Failed to initialize FSLogger: " << globalFSOptions._logFile << std::endl;
			return false;
		} else {
			log() << "Initialized FS Logger" << endl;
		}
	}

	if (!globalFSOptions._host) {
		globalFSOptions._host = "localhost";
	}

	if (!globalFSOptions._port) {
		globalFSOptions._port = 27017;
	} else if (globalFSOptions._port < 0 || globalFSOptions._port > 65535) {
		std::cerr << "Port number cannot be outside valid range [1..65535]: found to be " << globalFSOptions._port;
	}

	globalFSOptions._hostAndPort = mongo::HostAndPort(globalFSOptions._host, globalFSOptions._port);
	mongo::ScopedDbConnection sdc(mongo::ConnectionString(globalFSOptions._hostAndPort));
	mongo::DBClientBase& dbc = sdc.conn();
	std::cout << "Connection to mongodb succeeded {ConnId: " << dbc.getConnectionId() 
			<< ", WireVersion: {Min: " << dbc.getMinWireVersion() << ", Max: " << dbc.getMaxWireVersion() << "}"
			<< ", IsConnected: " << dbc.isStillConnected() << ", SO-timeout: " << dbc.getSoTimeout()
			<< ", Type: " << (long)dbc.type() << "}" << std::endl;

	return true;
}
