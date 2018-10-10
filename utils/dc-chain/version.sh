#!/bin/sh

# This shell script extract versions from Makefile
# It's used in ./download.sh, ./unpack.sh and ./cleanup.sh

function get_make_var()
{
	TOKEN="$1="
	VARIABLE=$(cat 'Makefile' | grep "$TOKEN" | sed "s/$TOKEN//g")
	echo "${VARIABLE}"
}

export GCC_VER=`get_make_var gcc_ver`
export BINUTILS_VER=`get_make_var binutils_ver`
export NEWLIB_VER=`get_make_var newlib_ver`
export GMP_VER=`get_make_var gmp_ver`
export MPFR_VER=`get_make_var mpfr_ver`
export MPC_VER=`get_make_var mpc_ver`
export GDB_VER=`get_make_var gdb_ver`
export INSIGHT_VER=`get_make_var insight_ver`
