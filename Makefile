CXX=g++
CXXFLAGS=-Wall -g -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 -I${HOME}/mongo-client-install/include -DMONGO_EXPOSE_MACROS
LIBS=-lfuse -lulockmgr -lmongoclient -lboost_system -lboost_filesystem -lboost_thread -lpthread
LDFLAGS=-L${HOME}/mongo-client-install/lib

#LDOPTS=-lmongoclient -lfuse_ino64 -lboost_thread-mt -lboost_filesystem-mt -lboost_system-mt

all: mgridfs

rebuild: clean all

clean:
	rm -f *.o mgridfs

mgridfs: file_handle.o utils.o fs_options.o file_meta_ops.o fs_meta_ops.o dir_meta_ops.o fs_conn_info.o fs_logger.o main.o
	${CXX} ${CXXFLAGS} ${LDFLAGS} $^ ${LIBS} -o $@
