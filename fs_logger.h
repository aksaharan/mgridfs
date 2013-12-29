#ifndef mgridfs_fs_logger_h
#define mgridfs_fs_logger_h

#include <string>
#include <fstream>
#include <sstream>
#include <boost/utility.hpp>

using namespace std;

namespace mgridfs {

typedef enum {
	LL_INVALID,
	LL_TRACE,
	LL_DEBUG,
	LL_INFO,
	LL_WARN,
	LL_ERROR,
	LL_FATAL,
	LL_NONE,
} LogLevel;


class FSLogDestination : protected boost::noncopyable {
public:
	FSLogDestination() { }
	~FSLogDestination() { }

	virtual bool logAll(LogLevel ll, const string& logMessage) = 0;
};

class FSLogManager : protected boost::noncopyable {
public:
	static FSLogManager& get();

	LogLevel getLogLevel() const { return _ll; }
	void setLogLevel(LogLevel ll) { _ll = ll; }

	const string& logLevelToString(LogLevel ll) const;
	LogLevel stringToLogLevel(const string& logLevel) const;

	bool logAll(LogLevel ll, const string& logMessage);

	bool registerDestination(FSLogDestination* logDestination);

private:
	FSLogManager();
	~FSLogManager();

	LogLevel _ll; // Default log level at the process level
	FSLogDestination* _logDestination; // Should be possible to extend to multiple log facilities
};

class FSLogStream {
public:
	FSLogStream(LogLevel ll = LL_INFO);
	FSLogStream(const FSLogStream& fsLogStream);
	~FSLogStream();

	inline FSLogStream& operator<<(const string& s) {
		if (_ll >= _enabledLL) {
			_dirty = true;
			*this->_os << s;
		}
		return *this;
	}

	inline FSLogStream& operator<<(char c) {
		if (_ll >= _enabledLL) {
			_dirty = true;
			*this->_os << c;
		}
		return *this;
	}

	inline FSLogStream& operator<<(int i) {
		if (_ll >= _enabledLL) {
			_dirty = true;
			*this->_os << i;
		}
		return *this;
	}

	inline FSLogStream& operator<<(unsigned int ui) {
		if (_ll >= _enabledLL) {
			_dirty = true;
			*this->_os << ui;
		}
		return *this;
	}

	inline FSLogStream& operator<<(long l) {
		if (_ll >= _enabledLL) {
			_dirty = true;
			*this->_os << l;
		}
		return *this;
	}

	inline FSLogStream& operator<<(unsigned long l) {
		if (_ll >= _enabledLL) {
			_dirty = true;
			*this->_os << l;
		}
		return *this;
	}

	inline FSLogStream& operator<<(long long ll) {
		if (_ll >= _enabledLL) {
			_dirty = true;
			*this->_os << ll;
		}
		return *this;
	}

	inline FSLogStream& operator<<(unsigned long long ll) {
		if (_ll >= _enabledLL) {
			_dirty = true;
			*this->_os << ll;
		}
		return *this;
	}

	inline FSLogStream& operator<<(short s) {
		if (_ll >= _enabledLL) {
			_dirty = true;
			*this->_os << s;
		}
		return *this;
	}

	inline FSLogStream& operator<<(float f) {
		if (_ll >= _enabledLL) {
			_dirty = true;
			*this->_os << f;
		}
		return *this;
	}

	inline FSLogStream& operator<<(double d) {
		if (_ll >= _enabledLL) {
			_dirty = true;
			*this->_os << d;
		}
		return *this;
	}

	inline FSLogStream& operator<<(bool b) {
		if (_ll >= _enabledLL) {
			_dirty = true;
			*this->_os << b;
		}
		return *this;
	}

	inline FSLogStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
		if (_ll >= _enabledLL) {
			_dirty = true;
			*this->_os << manip;
		}
		return *this;
	}

	inline FSLogStream& operator<<(std::ios_base& (*manip)(std::ios_base&)) {
		if (_ll >= _enabledLL) {
			_dirty = true;
			*this->_os << manip;
		}
		return *this;
	}

	template<typename T>
	inline FSLogStream& operator<<(T t) {
		if (_ll >= _enabledLL) {
			_dirty = true;
			*this->_os << t.toString();
		}
		return *this;
	}

private:
	LogLevel _ll, _enabledLL;
	bool _dirty;
	ostringstream* _os;

	FSLogStream& operator=(const FSLogStream&);
};

template<>
inline FSLogStream& FSLogStream::operator<< <const char*>(const char* s) {
	if (_ll >= _enabledLL) {
		_dirty = true;
		*this->_os << s;
	}
	return *this;
}

template<>
inline FSLogStream& FSLogStream::operator<< <char*>(char* s) {
	return operator<< <const char*>(s);
}


/* 
 * define various log level stream utility functions
*/
inline FSLogStream trace() { return FSLogStream(LL_TRACE); }
inline FSLogStream debug() { return FSLogStream(LL_DEBUG); }
inline FSLogStream info() { return FSLogStream(LL_INFO); }
inline FSLogStream warn() { return FSLogStream(LL_WARN); }
inline FSLogStream error() { return FSLogStream(LL_ERROR); }
inline FSLogStream fatal() { return FSLogStream(LL_FATAL); }

class FSLogFile : public FSLogDestination {
public:
	FSLogFile(const string& filename);
	~FSLogFile();

	virtual bool logAll(LogLevel ll, const string& logMessage);

private:
	string _filename;
	ofstream _logFile;
};

}

#endif
