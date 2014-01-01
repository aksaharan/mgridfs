#ifndef mgridfs_local_gridfs_h
#define mgridfs_local_gridfs_h

#include "local_grid_file.h"

#include <string>
#include <boost/utility.hpp>

using namespace std;

namespace mgridfs {

class LocalGridFS : protected boost::noncopyable {
public:
	static LocalGridFS& get();

	LocalGridFile* findByName(const string& file);

private:
	LocalGridFS();
	~LocalGridFS();
};

}

#endif
