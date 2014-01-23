import argparse

def get_parsed_args():
	p = argparse.ArgumentParser()
	Runner.build_arg_parser(p)
	GridFSPerfRunner.build_arg_parser(p)
	FSOverGridFSPerfRunner.build_arg_parser(p)
	return p.parse_args()
	
def run():
	parser = argparse.ArgumentParser


class Runner(object):
	"""Perf runner base class that defines common behaviour amont the classes
	"""

	def __init__(self):
		print "Called for ", __file__, "::__init__"
	
	def run(self):
		print "This is in " + __file__ + "::run"
		return False

	@classmethod
	def build_arg_parser(self, p):
		p.add_argument("-v", "--verbose", help="run in verbose mode")
		p.add_argument("--run", help="type of perf run i.e. [ mgridfs | gridfs ].", choices=["mgridfs", "gridfs"])

		p.add_argument("--src", help="source file directory from where to pick test files. Ideally this would be local file (not the mgridfs mounted one).")
		p.add_argument("--files", help="comma separated list of files to be used for both reading and writing.")
		pass
		

class FSOverGridFSPerfRunner(Runner):
	"""Class to run performance tests for MGridFS exported FS
	"""

	def __init__(self):
		print "Called for ", __file__, "::__init__"
	
	def run(self):
		print "This is in " + __file__ + "::run"
		return True

	@classmethod
	def build_arg_parser(self, p):
		p.add_argument("--dest", help="destination file directory to copy the files to. This should be mgridfs mounted path.")



#########################################################
class GridFSPerfRunner(Runner):
	"""Class to run performance tests for file operations directly on MongoDB-GridFS
	"""

	def __init__(self):
		print "Called for ", __file__, "::__init__"

	@classmethod
	def build_arg_parser(self, p):
		p.add_argument("--server", help="mongodb server.")
		p.add_argument("--port", type=int, help="mongodb port.")
		p.add_argument("--db", help="mongodb database name.")
		p.add_argument("--collprefix", help="mongodb collection prefix to be used for files and chunks collections.")

