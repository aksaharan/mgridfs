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

	FSLogStream& operator<<(const string& s);
	FSLogStream& operator<<(const char* const s);
	FSLogStream& operator<<(char c);
	FSLogStream& operator<<(int i);
	FSLogStream& operator<<(unsigned int ui);
	FSLogStream& operator<<(long l);
	FSLogStream& operator<<(unsigned long l);
	FSLogStream& operator<<(long long ll);
	FSLogStream& operator<<(unsigned long long ll);
	FSLogStream& operator<<(short s);
	FSLogStream& operator<<(float f);
	FSLogStream& operator<<(double d);

	template <typename T>
	FSLogStream& operator<<(const T& x) {
		*this->_os << x.toString();
		return *this;
	}

	FSLogStream& operator<<(std::ostream& (*manip)(std::ostream&));
	FSLogStream& operator<<(std::ios_base& (*manip)(std::ios_base&));

private:
	LogLevel _ll, _enabledLL;
	bool _dirty;
	ostringstream* _os;

	FSLogStream& operator=(const FSLogStream&);
};

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
