#!/usr/bin/env python2

import re
import os
import sys

if len(sys.argv) is not 5:
	print "You need to specify a filename, exiting..."
	exit(-1)

linewidth = 75
filename = sys.argv[1]
author = sys.argv[2]
year = sys.argv[3]
project = sys.argv[4].upper()

if not os.path.isfile(filename):
	print "Invalid file, exiting..."
	exit(-1)

#
# read file, check for opening /* in the first five lines
#
f = open(filename, 'r')
linenr = 0
headerflag = False
startline = 0
endline = 0
while True:
	line = f.readline()
	if not line:
		break
	if not headerflag:
		if line[0:2] == '/*':
			startline = linenr
			headerflag = True
			continue
	if headerflag:
		if re.search('\*/', line):
			endline = linenr
			f.close()
			break

	linenr += 1

print "Removing block from %i - %i" % (startline, endline)

#
# prepare new block
#
f = open(os.path.dirname(os.path.abspath(__file__)) + '/gnu3.txt', 'r')
text = f.readlines()
f.close()

header = []
for i, line in enumerate(text):
	if i is 0 or i > len(text)-2:
		header.append(line[0:-1] + '\n')
		continue
	line = re.sub('\<FILENAME\>', os.path.basename(filename), line)
	line = re.sub('\<PROJECT_CAPITAL\>', project, line)
	line = re.sub('\<AUTHOR\>', author, line)
	line = re.sub('\<YEAR\>', year, line)
	line = re.sub('\s+\*\n$', '', line)
	nrspaces = (linewidth - len(line) - 1)
	line = line + (' ' * nrspaces) + '*\n'
	header.append(line)

#
# write to file block
#
newfile = []
f = open(filename, 'r')
lines = f.readlines()
f.close()

headerline = 0
for i, line in enumerate(lines):
	if i is endline + 1:
		for l in header:
			newfile.append(l)
	if i >= startline and i<= endline+1:
		continue
	else:
		newfile.append(line)

f = open(filename, 'w')
for newline in newfile:
	f.write(newline)
f.close()