#!/bin/sh

usage() {
	echo 'genexportstubs.sh <inpfile> <outpfile>'
}

# Check for enough parameters
if [ $# != 2 ]; then
	echo "Not enough parameters: need 2, got $#"
	usage
	exit 1
fi

inpfile=$1
outpfile=$2

# Get the list of export names
names=`cat $inpfile | grep -v '^#' | grep -v '^include ' | grep -v '^$' | sort`

# Write out a header
rm -f $outpfile
echo '/* This is a generated file, do not edit!! */' > $outpfile
echo '#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"' >> $outpfile

# Write out dummy variables as sym placeholders
for i in $names; do
	echo "int $i;" >> $outpfile
done


