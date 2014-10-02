mgridfs
=======
This is a userspace file system to manage files in MongoDB-GridFS.


Operations tested to be working
================================
- ls
- ls as different user
- mkdir -p 
- df -h 
- du
- chmod
- touch - using utime (only for existing file / directory)
- chown / by user / by group / by user and group
- lsof
- copy
- move (only single file)
- >, >>

Example mount
================
- Run in single-threaded mode for now until the system is made thread-safe
Access for everyone - sudo ./mgridfs -s --host=localhost --port=27017 --db=rest --collprefix=ls --logfile=fs.log -o allow_other,default_permissions dummy
Access only for mount user - ./mgridfs -s --host=localhost --port=27017 --db=rest --collprefix=ls --logfile=fs.log -dummy

For specific options that you would like to use with mgridfs, check "mgridfs --help" option on the command

Known issues
===============
- All known issues with using mongodb in a distributed environment i.e. lack of ACIDity across multiple documents
- Un-implemented features for file-system
- Thread-safety for multi-threaded FUSE usage

TODO:
==============
- fsck.mgridfs to fix hierarchy / permissions on the hierarchy. Data consistency checks cannot be done by this process and rather should be done someway in the MongoDB GridFS itself.

DEBUG
========
Using valgind, use something as follows (assuming mgridfs is in current directory and the mount point is dummy):
valgrind --log-file=valgrind.log --tool=memcheck --leak-check=full --show-reachable=no --undef-value-errors=yes --track-origins=yes ./mgridfs -s --host=localhost --port=27017 --db=rest --collprefix=ls --loglevel=trace -o default_permissions -d dummy

Useful links for FUSE / Mongo GridFS
========================================
- http://fuse.sourceforge.net/
- https://www.kernel.org/doc/Documentation/filesystems/fuse.txt
- http://www.cs.hmc.edu/~geoff/classes/hmc.cs135.201001/homework/fuse/fuse_doc.html
- http://docs.mongodb.org/manual/core/gridfs/
- http://docs.mongodb.org/ecosystem/drivers/cpp/
