#!/bin/sh

cd /workspace

cp doc/environ.sh.sample environ.sh
sed -i "s/KOS_BASE=.*$/KOS_BASE=\\/workspace/" environ.sh
. environ.sh
make
