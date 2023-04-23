#!/bin/sh

usage() {
	echo 'genexports.sh <inpfile> <outpfile> <symbolname>'
}

# Check for enough parameters
if [ $# != 3 ]; then
	echo "Not enough parameters: need 3, got $#"
	usage
	exit 1
fi

inpfile=$1
outpfile=$2
outpsym=$3

includes=`cat $inpfile | grep '^include ' | cut -d' ' -f2 | sort`

# Get the list of export names
names=`cat $inpfile | grep -v '^#' | grep -v '^include ' | grep -v '^$' | sort`

# Write out a header
rm -f $outpfile
echo '/* This is a generated file, do not edit!! */' > $outpfile
echo '#define __EXPORTS_FILE' >> $outpfile

for i in $includes; do
	echo "#include <$i>" >> $outpfile
done

# Now write out the sym table
echo "export_sym_t ${outpsym}[] = {" >> $outpfile
for i in $names; do
	echo "	{ \"$i\", (unsigned long)(&$i) }," >> $outpfile
done

echo "	{ 0, 0 }" >> $outpfile
echo "};" >> $outpfile

