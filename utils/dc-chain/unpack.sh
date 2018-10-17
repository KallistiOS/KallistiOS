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

# Clean up from any old builds.
rm -rf binutils-$BINUTILS_VER gcc-$GCC_VER newlib-$NEWLIB_VER
rm -rf gmp-$GMP_VER mpfr-$MPFR_VER mpc-$MPC_VER

# Unpack everything.
tar xf binutils-$BINUTILS_VER.tar.xz || exit 1
tar xf gcc-$GCC_VER.tar.bz2 || exit 1
tar xf newlib-$NEWLIB_VER.tar.gz || exit 1

# Unpack the GCC dependencies and move them into their required locations.
if [ -n "$GMP_VER" ]; then
    tar jxf gmp-$GMP_VER.tar.bz2 || exit 1
    mv gmp-$GMP_VER gcc-$GCC_VER/gmp
fi

if [ -n "$MPFR_VER" ]; then
    tar jxf mpfr-$MPFR_VER.tar.bz2 || exit 1
    mv mpfr-$MPFR_VER gcc-$GCC_VER/mpfr
fi

if [ -n "$MPC_VER" ]; then
    tar zxf mpc-$MPC_VER.tar.gz || exit 1
    mv mpc-$MPC_VER gcc-$GCC_VER/mpc
fi
