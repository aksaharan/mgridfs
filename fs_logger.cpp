#include "fs_logger.h"

#include <iostream>

using namespace std;
using namespace mgridfs;

mgridfs::FSLogger mgridfs::fsLogger;

FSLogger::FSLogger() {
}

FSLogger::~FSLogger() {
	if (_logFile.is_open()) {
		_logFile << "Closing MongoDB-GridFS" << std::endl;
		_logFile.close();
	}
}

bool mgridfs::FSLogger::initialize(const string& name) {
	if (fsLogger._logFile.is_open()) {
		cerr << "Logfile is already opened, will not try opening new file [File: " << name << "]." << endl;
		return false;
	}

	fsLogger._logFile.open(name.c_str());
	if (!fsLogger._logFile.good()) {
		cerr << "Failed to open logfile: " << name << endl;
		return false;
	}

	cout << "Opened logfile: " << name << endl;
	return true;
}

FSLogger& FSLogger::operator<<(const string& s) {
	this->_logFile << s;
	return *this;
}

FSLogger& FSLogger::operator<<(const char* const s) {
	this->_logFile << s;
	return *this;
}

FSLogger& FSLogger::operator<<(char c) {
	this->_logFile << c;
	return *this;
}

FSLogger& FSLogger::operator<<(int i) {
	this->_logFile << i;
	return *this;
}

FSLogger& FSLogger::operator<<(unsigned int ui) {
	this->_logFile << ui;
	return *this;
}

FSLogger& FSLogger::operator<<(long l) {
	this->_logFile << l;
	return *this;
}

FSLogger& FSLogger::operator<<(long long ll) {
	this->_logFile << ll;
	return *this;
}

FSLogger& FSLogger::operator<<(short s) {
	this->_logFile << s;
	return *this;
}

FSLogger& FSLogger::operator<<(float f) {
	this->_logFile << f;
	return *this;
}

FSLogger& FSLogger::operator<<(double d) {
	this->_logFile << d;
	return *this;
}
