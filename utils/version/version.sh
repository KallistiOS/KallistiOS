#!/bin/sh

# Little version release util for KOS
# (c)2000-2002 Megan Potter

# Call this program on each file to substitute in the proper version code
# for the version header. This works with find|xargs.

VERSION=$1
shift
for i in $*; do
	echo processing $i to version $VERSION
	sed -e "s/##version##/$VERSION/g" < $i > /tmp/tmp1.out
	sed -e "s/\\\\#\\\\#version\\\\#\\\\#/$VERSION/g" < /tmp/tmp1.out > $i
	rm -f /tmp/tmp1.out
done



