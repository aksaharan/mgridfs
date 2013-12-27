#include "fs_logger.h"

#include <sstream>
#include <fstream>
#include <iostream>

#include <boost/bimap.hpp>
#include <mongo/util/time_support.h>

using namespace std;
using namespace mgridfs;

namespace {
	//typedef boost::bimap<LogLevel, string> LogLevelToStringMap;
	typedef boost::bimap<LogLevel, string> LogLevelToStringMap;
	LogLevelToStringMap logLevelToStringMap;

	const string LL_INVALID_STR = "INVALID";
}

FSLogManager::FSLogManager()
	: _ll(LL_TRACE), _logDestination(NULL) {

	logLevelToStringMap.insert(LogLevelToStringMap::value_type(LL_INVALID, LL_INVALID_STR));
	logLevelToStringMap.insert(LogLevelToStringMap::value_type(LL_TRACE, "TRACE"));
	logLevelToStringMap.insert(LogLevelToStringMap::value_type(LL_DEBUG, "DEBUG"));
	logLevelToStringMap.insert(LogLevelToStringMap::value_type(LL_INFO, "INFO"));
	logLevelToStringMap.insert(LogLevelToStringMap::value_type(LL_WARN, "WARN"));
	logLevelToStringMap.insert(LogLevelToStringMap::value_type(LL_ERROR, "ERROR"));
	logLevelToStringMap.insert(LogLevelToStringMap::value_type(LL_FATAL, "FATAL"));
	logLevelToStringMap.insert(LogLevelToStringMap::value_type(LL_NONE, "NONE"));
}

FSLogManager::~FSLogManager() {
	if (_logDestination) {
		delete _logDestination;
		_logDestination = NULL;
	}
}

FSLogManager& FSLogManager::get() {
	static FSLogManager instance;
	return instance;
}

const string& FSLogManager::logLevelToString(LogLevel ll) const {
	LogLevelToStringMap::left_map::const_iterator pIt = logLevelToStringMap.left.find(ll);
	if (pIt != logLevelToStringMap.left.end()) {
		return pIt->second;
	}
	return LL_INVALID_STR;
}

LogLevel FSLogManager::stringToLogLevel(const string& logLevel) const {
	LogLevelToStringMap::right_map::const_iterator pIt = logLevelToStringMap.right.find(logLevel);
	if (pIt != logLevelToStringMap.right.end()) {
		return pIt->second;
	}
	return LL_INVALID;
}

bool FSLogManager::logAll(LogLevel ll, const string& logMessage) {
	if (ll >= _ll) {
		if (_logDestination) {
			_logDestination->logAll(ll, logMessage);
		} else {
			cout << logMessage << flush;
		}
	}
	return true;
}

bool FSLogManager::registerDestination(FSLogDestination* logDestination) {
	// TODO: Make thread-safe if need to be enabled for multi-threaded
	FSLogDestination* temp = _logDestination;
	_logDestination = logDestination;
	if (temp) {
		delete temp;
	}

	return true;
}

FSLogFile::FSLogFile(const string& filename) 
	: _filename(filename), _logFile(filename.c_str()) {
	info() << "Opened MongoDB-GridFS log file {file: " << filename << "}" << endl;
}

FSLogFile::~FSLogFile() {
	if (_logFile.is_open()) {
		info() << "Closing MongoDB-GridFS log file" << endl;
		_logFile.close();
	}
}

bool FSLogFile::logAll(LogLevel ll, const string& logMessage) {
	_logFile << logMessage << flush;
	return true;
}

FSLogStream::FSLogStream(LogLevel ll)
	: _ll(ll), _enabledLL(FSLogManager::get().getLogLevel()), _dirty(false), _os(new ostringstream()) {
	*_os << dateToCtimeString(mongo::jsTime()) << " [thread-" << pthread_self() << "] " << FSLogManager::get().logLevelToString(ll) << " ";
}

FSLogStream::FSLogStream(const FSLogStream& fsLogStream)
	: _ll(fsLogStream._ll), _enabledLL(fsLogStream._enabledLL), _dirty(false), _os(new ostringstream()) {
}

FSLogStream::~FSLogStream() {
	if (_os) {
		if (_ll >= _enabledLL && _dirty) {
			FSLogManager::get().logAll(_ll, _os->str());
		}
		delete _os;
		_os = NULL;
	}
}

FSLogStream& FSLogStream::operator<<(const string& s) {
	if (_ll >= _enabledLL) {
		_dirty = true;
		*this->_os << s;
	}
	return *this;
}

FSLogStream& FSLogStream::operator<<(const char* const s) {
	if (_ll >= _enabledLL) {
		_dirty = true;
		*this->_os << s;
	}
	return *this;
}

FSLogStream& FSLogStream::operator<<(char c) {
	if (_ll >= _enabledLL) {
		_dirty = true;
		*this->_os << c;
	}
	return *this;
}

FSLogStream& FSLogStream::operator<<(int i) {
	if (_ll >= _enabledLL) {
		_dirty = true;
		*this->_os << i;
	}
	return *this;
}

FSLogStream& FSLogStream::operator<<(unsigned int ui) {
	if (_ll >= _enabledLL) {
		_dirty = true;
		*this->_os << ui;
	}
	return *this;
}

FSLogStream& FSLogStream::operator<<(long l) {
	if (_ll >= _enabledLL) {
		_dirty = true;
		*this->_os << l;
	}
	return *this;
}

FSLogStream& FSLogStream::operator<<(unsigned long l) {
	if (_ll >= _enabledLL) {
		_dirty = true;
		*this->_os << l;
	}
	return *this;
}

FSLogStream& FSLogStream::operator<<(long long ll) {
	if (_ll >= _enabledLL) {
		_dirty = true;
		*this->_os << ll;
	}
	return *this;
}

FSLogStream& FSLogStream::operator<<(unsigned long long ll) {
	if (_ll >= _enabledLL) {
		_dirty = true;
		*this->_os << ll;
	}
	return *this;
}

FSLogStream& FSLogStream::operator<<(short s) {
	if (_ll >= _enabledLL) {
		_dirty = true;
		*this->_os << s;
	}
	return *this;
}

FSLogStream& FSLogStream::operator<<(float f) {
	if (_ll >= _enabledLL) {
		_dirty = true;
		*this->_os << f;
	}
	return *this;
}

FSLogStream& FSLogStream::operator<<(double d) {
	if (_ll >= _enabledLL) {
		_dirty = true;
		*this->_os << d;
	}
	return *this;
}

FSLogStream& FSLogStream::operator<<(std::ostream& (*manip)(std::ostream&)) {
	if (_ll >= _enabledLL) {
		_dirty = true;
		*this->_os << manip;
	}
	return *this;
}

FSLogStream& FSLogStream::operator<<(std::ios_base& (*manip)(std::ios_base&)) {
	if (_ll >= _enabledLL) {
		_dirty = true;
		*this->_os << manip;
	}
	return *this;
}
