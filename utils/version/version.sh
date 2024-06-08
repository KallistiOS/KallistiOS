#!/bin/sh

# Little version release util for KOS
# Copyright (C) 2000-2002 Megan Potter
# Copyright (C) 2024 Falco Girgis

# Call this program on each file to substitute in the proper version code
# for the version header. This works with find|xargs.
#
# NOTE: Typically you do not want to explicitly provide a version number
#       to this script, as it will automatically source the version
# 		information from the KOS environment variables which come from
#       include/kos/version.h, which should always be the main source of
#       truth for a release version!

VERSION=$1
if [[ -z "$var" ]]; then
	VERSION="$KOS_VERSION"
fi
shift
for i in $*; do
	echo processing $i to version $VERSION
	sed -e "s/##version##/$VERSION/g" < $i > /tmp/tmp1.out
	sed -e "s/\\\\#\\\\#version\\\\#\\\\#/$VERSION/g" < /tmp/tmp1.out > $i
	rm -f /tmp/tmp1.out
done



