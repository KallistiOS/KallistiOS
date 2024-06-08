#!/bin/sh

# Little version release util for KOS
# Copyright (C) 2000-2002 Megan Potter
# Copyright (C) 2024 Falco Girgis

# Call this program on each file to substitute in the proper version code
# for the version header. This works with find|xargs.
#
# NOTE: The version ID comes from environ_base.sh environ variables which
#       themselves are sourced from $KOS_BASE/include/kos/version.h as the
#		ultimate source of truth for KOS's version.

VERSION=$KOS_VERSION
if [ -z "$VERSION" ]; then
	echo "Failed to find KOS_VERSION environment variable!"
	exit 1
fi
if [ -z "$1" ]; then
	echo "No file(s) specified!"
	exit 1
fi
for i in $@; do
	echo processing $i to version $VERSION
	sed -e "s/##version##/$VERSION/g" < $i > /tmp/tmp1.out
	sed -e "s/\\\\#\\\\#version\\\\#\\\\#/$VERSION/g" < /tmp/tmp1.out > $i
	rm -f /tmp/tmp1.out
done



