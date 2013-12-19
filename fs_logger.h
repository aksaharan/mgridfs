#ifndef fs_logger_h
#define fs_logger_h

#include <string>
#include <fstream>
#include <sstream>
#include <boost/utility.hpp>

using namespace std;

namespace mgridfs {

class FSLogger : protected boost::noncopyable {
public:
	FSLogger();
	~FSLogger();

	static bool initialize(const string& name);

	FSLogger& operator<<(const string& s);
	FSLogger& operator<<(const char* const s);
	FSLogger& operator<<(char c);
	FSLogger& operator<<(int i);
	FSLogger& operator<<(unsigned int ui);
	FSLogger& operator<<(long l);
	FSLogger& operator<<(long long ll);
	FSLogger& operator<<(short s);
	FSLogger& operator<<(float f);
	FSLogger& operator<<(double d);

	template <typename T>
	FSLogger& operator<<(const T& x) {
		this->_logFile << x.toString();
		return *this;
	}

	inline FSLogger& operator<< (std::ostream& ( *manip )(std::ostream&)) {
		this->_logFile << manip;
		return *this;
	}

	inline FSLogger& operator<< (std::ios_base& (*manip)(std::ios_base&)) {
		this->_logFile << manip;
		return *this;
	}


	bool _default;
	ofstream _logFile;
};

extern FSLogger fsLogger;

inline FSLogger& trace() { return fsLogger; }
inline FSLogger& debug() { return fsLogger; }
inline FSLogger& log() { return fsLogger; }
inline FSLogger& warn() { return fsLogger; }
inline FSLogger& error() { return fsLogger; }
inline FSLogger& fatal() { return fsLogger; }

class FSLogStream {
public:
	FSLogStream(string logLevel);
	~FSLogStream();

private:
	stringstream os;
};

}

#endif
