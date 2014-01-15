#ifndef mgridfs_local_gridfs_h
#define mgridfs_local_gridfs_h

#include <map>
#include <string>
#include <boost/utility.hpp>

using namespace std;

namespace mgridfs {

class LocalGridFile;

class LocalGridFS : protected boost::noncopyable {
public:
	typedef map<string, LocalGridFile*> LocalGridFileMap;

	static LocalGridFS& get();

	LocalGridFile* findByName(const string& filename);
	LocalGridFile* createFile(const string& filename);
	bool releaseFile(const string& filename);

	bool releaseAllFiles(bool flushAll);

private:
	LocalGridFS();
	~LocalGridFS();

	static LocalGridFileMap _localGridFileMap;
};

}

#endif
