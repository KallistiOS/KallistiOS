#!/bin/sh

cd /workspace

if [ -d kernel/arch/$1 ] ; then
	apk --update add --no-cache coreutils

	cp doc/environ.sh.sample environ.sh
	sed -i "s/KOS_BASE=.*$/KOS_BASE=\\/workspace/" environ.sh
	. environ.sh
	make
fi
