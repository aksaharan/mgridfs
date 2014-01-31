import sys
import os
import stat
import time
import io

import datetime
from datetime import datetime
from datetime import timedelta

import pymongo
import gridfs

import traceback
import argparse

"""
#!/usr/bin/env python

import pymongo
import gridfs

db = pymongo.MongoClient("localhost", 27017).test
fs = gridfs.GridFS(db)

fs.put("Hello World")
fs.put("Hello World", filename="hello-ji-file.txt", otherprop="ThisisOtherProp", metadata={})
fs.put("Hello World", filename="hello-ji-file-metadata.txt", otherprop="ThisisPropMeta", 
		metadata={"directory": "/sample/directory/final", "owner": 123, "group": 564, "permission": 0644})
"""

#mgridfs -s --host=localhost --port=27017 --db=rest --collprefix=ls --logfile=fs.log --loglevel=trace --memChunkSize=1024 --maxMemFileChunks=100 dummy

def getArgumentParser():
	p = argparse.ArgumentParser()

	Runner.buildArgParser(p)
	GridFSPerfRunner.buildArgParser(p)
	FSOverGridFSPerfRunner.buildArgParser(p)
	return p
	
def run(p):
	runners = [];
	# Add MGridFS runner for read/write via mgridfs mounted file system
	if (p.run == "mgridfs" or p.run == "both"):
		runner = FSOverGridFSPerfRunner(p)
		if (not runner.isRunnable()):
			print >> sys.stderr, "ERROR: Instance mgridfs is not runnable..."
			return -1

		runners.append(runner)

	# Add GridFS runner for read/write directly to GridFS
	if (p.run == "gridfs" or p.run == "both"):
		runner = GridFSPerfRunner(p)
		if (not runner.isRunnable()):
			print >> sys.stderr, "ERROR: Instance gridfs is not runnable..."
			return -1

		runners.append(runner)
	
	if (len(runners) == 0):
		#TODO: use standard logging library
		print >> sys.stderr, "ERROR: no runners configured, will abort."
		return -1

	completed = 0
	print "INFO: Runners configured for execution: ", len(runners)
	for runner in runners:
		print "Running runner: ", runner
		try:
			if (runner.run()):
				++completed;
		except Exception, e:
			print "ERROR: " + str(e)
			traceback.print_exc()
	
	return (len(runners) - completed)


class Runner(object):
	"""Perf runner base class that defines common behaviour amont the classes
	"""
	_files = []
	_outFile = None
	_outFilename = ""
	_srcdir = ""
	_loopCount = None
	_filesAvailableForRead = []

	@classmethod
	def buildArgParser(self, p):
		p.add_argument("-v", "--verbose", help="run in verbose mode", default = True)
		p.add_argument("--run", help="type of perf run i.e. [ mgridfs | gridfs ].", choices=["mgridfs", "gridfs", "both"])

		p.add_argument("--srcdir", help="source file directory from where to pick test files. Ideally this would be local file (not the mgridfs mounted one).",
			default = ".")
		p.add_argument("--files", help="comma separated list of files to be used for both reading and writing.")
		p.add_argument("--outfile", help="dump statistics to the specified out file. By default prints to the standard out.");
		p.add_argument("--loopcnt", help="loop count for each file run. By default set to 100.", type = int, default = 100);
		pass


	def __init__(self, p):
		print "Called for ", __name__, ".Runner.__init__ -> ", p
		if (p.files != None):
			self._files = p.files.split(",")

		self._srcdir = p.srcdir
		self._outFilename = p.outfile
		if (self._outFilename == None):
			self._outFile = sys.stdout
		else:
			self._outFile = open(self._outFilename, "w")

		self._loopCount = p.loopcnt


	def __str__(self):
		return __name__ + ".Runner"


	def isRunnable(self):
		if (len(self._files) <= 0):
			print >> sys.stderr, "No files specified for the run."
			return False

		if (self._outFile == None):
			return False

		if (self._srcdir == None):
			return False

		if (not os.path.isdir(self._srcdir)):
			return False

		return True


	def run(self):
		if (not self.isRunnable()):
			return False

		self.printStatHeader(["file", "op", "size", "repeat_count", "time (millisecs)"])

		for filename in self._files:
			if (not self.runFile(filename)):
				print "ERROR: Failed to run for file: ", filename
				continue

		self.printStatFooter([])


	def runFile(self):
		raise NotImplementedError(__name__ + ".runFile not implemented")


	def printStatHeader(self, headers):
		if (headers):
			print >> self._outFile, "\t".join(map(str, headers))


	def printStatFooter(self, footers):
		if (footers):
			print >> self._outFile, "\t".join(map(str, footers))


	def printStatRecord(self, stats):
		print >> self._outFile, "\t".join(map(str, stats))

	@staticmethod
	def fileExists(filename):
		try:
			fileStat = os.stat(filename)
		except:
			return False

		return True
		

class FSOverGridFSPerfRunner(Runner):
	"""Class to run performance tests for MGridFS exported FS
	"""

	@classmethod
	def buildArgParser(self, p):
		p.add_argument("--destdir", help="destination file directory to copy the files to. This should be mgridfs mounted path.")


	def __init__(self, p):
		print "Called for ", __name__, ".FSOverGridFSPerfRunner.__init__"
		super(FSOverGridFSPerfRunner, self).__init__(p)

		self._destdir = p.destdir

	
	def __str__(self):
		return __name__ + ".FSOverGridFSPerfRunner"


	def isRunnable(self):
		if (not super(FSOverGridFSPerfRunner, self).isRunnable()):
			return False

		if (self._destdir == None):
			return False

		if (not os.path.isdir(self._destdir)):
			return False

		return True;


	def runFile(self, filename):
		if (not self.runFileWrite(filename)):
			return False

		if (not self.runFileRead(filename)):
			return False

		return True


	def runFileRead(self, filename):
		fullFilename = os.path.join(self._destdir, filename)
		if (not self.fileExists(fullFilename)):
			print "ERROR: File is not accessible [", filename, "]"
			return False

		for i in range(self._loopCount):
			start = time.time()

			try:
				fileSize = 0
				with open(fullFilename, "r") as f:
					readBytes = f.read(4096 * 1024)
					while (len(readBytes) > 0):
						fileSize += len(readBytes)
						readBytes = f.read(4096 * 1024)
			except Exception, e:
				traceback.print_exc()
			else:
				print "Completed read test for the file from system [loop: {}, file: {}]".format(i, fullFilename)
				diffTime = (time.time() - start) * 1000
				self.printStatRecord([filename, "read", fileSize, 1, diffTime])

		return True


	def runFileWrite(self, filename):
		fullDestFilename = os.path.join(self._destdir, filename)
		fullSrcFilename = os.path.join(self._srcdir, filename)

		if (not self.fileExists(fullSrcFilename)):
			print "ERROR: Source file is not found [", fullSrcFilename, "]. will not proceed with this test."
			return False

		for i in range(self._loopCount):
			# Delete the target file before performing writes on it
			self.deleteFile(fullDestFilename)
			start = time.time()

			try:
				fileSize = 0
				with open(fullSrcFilename, "r") as fin:
					with open(fullDestFilename, "w") as fout:
						readBytes = fin.read(4096 * 1024)
						while (len(readBytes) > 0):
							fileSize = len(readBytes)
							fout.write(readBytes)
							readBytes = fin.read(4096 * 1024)
			except Exception, e:
				traceback.print_exc()
			else:
				print "Completed write test for the file from system {loop: ", i, ", srcfile: ", fullSrcFilename, ", destfile: ", fullDestFilename, "}"
				diffTime = (time.time() - start) * 1000
				self.printStatRecord([filename, "write", fileSize, 1, diffTime])

		return True


	def deleteFile(self, filename):
		try:
			fileStat = os.stat(fullSrcFilename)
			os.unlink(filename)
		except:
			pass

		return True


#########################################################
class GridFSPerfRunner(Runner):
	"""Class to run performance tests for file operations directly on MongoDB-GridFS
	"""

	@classmethod
	def buildArgParser(self, p):
		p.add_argument("--server", help="mongodb server.", default = "localhost")
		p.add_argument("--port", help="mongodb port.", type = int,  default = 27017)
		p.add_argument("--db", help="mongodb database name.", default = "test")
		p.add_argument("--collprefix", help="mongodb collection prefix to be used for files and chunks collections.", default = "fs")


	def __init__(self, p):
		print "Called for ", __name__, ".GridFSPerfRunner.__init__"
		super(GridFSPerfRunner, self).__init__(p)

		self._server = p.server
		self._port = p.port
		self._db = p.db
		self._collPrefix = p.collprefix


	def __str__(self):
		return __name__ + ".GridFSPerfRunner"


	def isRunnable(self):
		if (not super(GridFSPerfRunner, self).isRunnable()):
			return False

		if (not self._server or not self._port or not self._db or not self._collPrefix):
			return False

		return True;


	def runFile(self, filename):
		if (not self.runFileWrite(filename)):
			return False

		if (not self.runFileRead(filename)):
			return False

		return True


	def runFileRead(self, filename):
		gridFS = gridfs.GridFS(self.getDb(), self._collPrefix)
		if (not gridFS.exists()):
			print "File not found on GridFS, not performing read test for this file: ", filename
			return True

		for i in range(self._loopCount):
			start = time.time()

			try:
				with gridFS.get_version(filename) as f:
					fileSize = f.length
					readBytes = f.read(4096 * 1024)
					readLen = len(readBytes);
					while (len(readBytes) > 0):
						readBytes = f.read(4096 * 1024)
						readLen += len(readBytes)

					if (readLen != fileSize):
						print "FATAL: Bytes read != file size [file: {}, size: {}, read: {}]".format(filename, fileSize, readLen)

			except Exception, e:
				traceback.print_exc()
			else:
				print "Completed read test for the file from system [loop: {}, file: {}]".format(i, filename)
				diffTime = (time.time() - start) * 1000
				self.printStatRecord([filename, "read", fileSize, 1, diffTime])

		return True


	def runFileWrite(self, filename):
		fullSrcFilename = os.path.join(self._srcdir, filename)
		if (not self.fileExists(fullSrcFilename)):
			print "ERROR: Source file is not found [", fullSrcFilename, "]. will not proceed with this test."
			return False

		gridFS = gridfs.GridFS(self.getDb(), self._collPrefix)
		for i in range(self._loopCount):
			# Delete the target file before performing writes on it
			self.deleteGridFile(filename, gridFS)
			start = time.time()

			try:
				fileSize = 0
				with open(fullSrcFilename, "r") as fin:
					with gridFS.new_file(filename = filename) as fout:
						readBytes = fin.read(4096 * 1024)
						while (len(readBytes) > 0):
							fileSize += len(readBytes)
							fout.write(readBytes)
							readBytes = fin.read(4096 * 1024)
			except Exception, e:
				traceback.print_exc()
			else:
				print "Completed write test for the file from system {loop: ", i, ", srcfile: ", fullSrcFilename, ", destfile: ", filename, "}"
				diffTime = (time.time() - start) * 1000
				self.printStatRecord([filename, "write", fileSize, 1, diffTime])

		return True

	def getDb(self):
		dbc = pymongo.MongoClient(self._server,  self._port)
		return dbc[self._db]



	def deleteGridFile(self, filename, gridFS):
		count = 0
		while (gridFS.exists({"filename": filename})):
			fin = gridFS.get_version(filename)
			gridFS.delete(fin._id)
			count += 1

		if (count > 0):
			print "Deleted {} instance of '{}'".format(count, filename)
