(Work still in progress)
===================

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

Example mount
================
Access for everyone - sudo ./mgridfs --host=localhost --port=27017 --db=rest --collprefix=ls --logfile=fs.log -o allow_other,default_permissions dummy
Access only for mount user - ./mgridfs --host=localhost --port=27017 --db=rest --collprefix=ls --logfile=fs.log -dummy

Known issues
===============
- All known issues with using mongodb in a distributed environment i.e. lack of ACIDity across multiple documents
- Un-implemented features for file-system

TODO:
==============
- fsck.mgridfs to fix hierarchy / permissions on the hierarchy. Data consistency checks cannot be done by this process and rather should be done someway in the MongoDB GridFS itself.

Useful links for FUSE / Mongo GridFS
========================================
- http://fuse.sourceforge.net/
- https://www.kernel.org/doc/Documentation/filesystems/fuse.txt
- http://www.cs.hmc.edu/~geoff/classes/hmc.cs135.201001/homework/fuse/fuse_doc.html
- http://docs.mongodb.org/manual/core/gridfs/
- http://docs.mongodb.org/ecosystem/drivers/cpp/
