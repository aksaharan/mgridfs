CXX=g++
CXXFLAGS=-Wall -g -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 -I${HOME}/mongo-client-install/include -DMONGO_EXPOSE_MACROS
LIBS=-lfuse -lulockmgr -lmongoclient -lboost_system -lboost_filesystem -lboost_thread -lpthread
LDFLAGS=-L${HOME}/mongo-client-install/lib

#LDOPTS=-lmongoclient -lfuse_ino64 -lboost_thread-mt -lboost_filesystem-mt -lboost_system-mt
COMMON_OBJECTS=file_handle.o local_grid_file.o local_gridfs.o utils.o fs_options.o file_meta_ops.o fs_meta_ops.o dir_meta_ops.o \
fs_conn_info.o fs_logger.o

TEST_OBJECTS=${COMMON_OBJECTS}

APP_OBJECTS=${COMMON_OBJECTS} main.o
TEST_APP_OBJECTS=${TEST_OBJECTS} test_main.o


all: mgridfs mgridfs_test

rebuild: clean all

clean:
	rm -f *.o mgridfs mgridfs_test

mgridfs: ${APP_OBJECTS}
	${CXX} ${CXXFLAGS} ${LDFLAGS} $^ ${LIBS} -o $@

mgridfs_test: ${TEST_APP_OBJECTS}
	${CXX} ${CXXFLAGS} ${LDFLAGS} $^ ${LIBS} -o $@
