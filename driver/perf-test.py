#!/usr/bin/env python

import bson
import optparse
import os
import re
import shutil
import socket
import sys
import time
import traceback
import argparse

BASEPATH = os.path.dirname(__file__)
sys.path.insert(0, BASEPATH)

import mgridfs
from mgridfs import *


def main():
	print "This is test print from the main.....\n"
	runner = perfrunner.run()
	args = perfrunner.get_parsed_args()
	print args

if __name__ == "__main__":
	main()
