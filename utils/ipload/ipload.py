#!/usr/bin/env python

# ipload.py
# Quick and dirty Python-based ip uploader for use with Marcus' IPLOAD
# (c)2001 Megan Potter
#
# Note: works ok with Python 2.0, untested with Python 1.x
#
# Also, change the host below from 'dc' to whatever IP you use for your
#  Dreamcast, or add an alias for 'dc' to your hosts file.
#

import sys
from socket import *

if len(sys.argv) < 2:
	print "usage: ipload.py <filename>"
	sys.exit(0)

f = open(sys.argv[1], "r")
data = f.read()
f.close()

sock = socket(AF_INET, SOCK_STREAM)
try:
	sock.connect(('dc', 4711))
except:
	sock.connect('dc', 4711)
left = len(data)
while left > 0:
	sent = sock.send(data)
	data = data[sent:]
	left = left - sent
sock.close()

sys.exit(0)

