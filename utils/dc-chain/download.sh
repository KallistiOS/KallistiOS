#!/usr/bin/env bash

# Getting versions defined in Makefile
source ./version.sh

while [ "$1" != "" ]; do
    PARAM=`echo $1 | awk -F= '{print $1}'`
    case $PARAM in
        --no-gmp)
            unset GMP_VER
            ;;
        --no-mpfr)
            unset MPFR_VER
            ;;
        --no-mpc)
            unset MPC_VER
            ;;
        --no-deps)
            unset GMP_VER
            unset MPFR_VER
            unset MPC_VER
            ;;
        *)
            echo "ERROR: unknown parameter \"$PARAM\""
            exit 1
            ;;
    esac
    shift
done

# Retrieve the web downloader program available in this system.
if command -v wget >/dev/null 2>&1; then
	WEB_DOWNLOADER="wget -c"
elif command -v curl >/dev/null 2>&1; then
	WEB_DOWNLOADER="curl -C - -O"
	IS_CURL=1
else
	echo >&2 "You must have either wget or cURL installed to use this script!"
	exit 1
fi

# Download everything.
if [ -z "${CONFIG_GUESS_ONLY}" ]; then
	echo "Downloading Binutils $BINUTILS_VER..."
	${WEB_DOWNLOADER} ftp://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VER.tar.xz || exit 1

	echo "Downloading GCC $GCC_VER..."
	${WEB_DOWNLOADER} ftp://ftp.gnu.org/gnu/gcc/gcc-$GCC_VER/gcc-$GCC_VER.tar.bz2 || exit 1

	echo "Downloading Newlib $NEWLIB_VER..."
	${WEB_DOWNLOADER} ftp://sourceware.org/pub/newlib/newlib-$NEWLIB_VER.tar.gz || exit 1

	if [ -n "$GMP_VER" ]; then
		echo "Downloading GMP $GMP_VER..."
		${WEB_DOWNLOADER} ftp://gcc.gnu.org/pub/gcc/infrastructure/gmp-$GMP_VER.tar.bz2 || exit 1
	fi

	if [ -n "$MPFR_VER" ]; then
		echo "Downloading MPFR $MPFR_VER..."
		${WEB_DOWNLOADER} ftp://gcc.gnu.org/pub/gcc/infrastructure/mpfr-$MPFR_VER.tar.bz2 || exit 1
	fi

	if [ -n "$MPC_VER" ]; then
		echo "Downloading MPC $MPC_VER..."
		${WEB_DOWNLOADER} ftp://gcc.gnu.org/pub/gcc/infrastructure/mpc-$MPC_VER.tar.gz || exit 1
	fi
fi

echo "Done!"
