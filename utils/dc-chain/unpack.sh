#!/bin/sh

# These version numbers are all that should ever have to be changed.
export SH_GCC_VER=10.1.0
export ARM_GCC_VER=8.4.0
export BINUTILS_VER=2.34
export NEWLIB_VER=3.3.0
export GMP_VER=6.2.0
export MPFR_VER=4.0.2
export MPC_VER=1.1.0

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
rm -rf binutils-$BINUTILS_VER gcc-$SH_GCC_VER gcc-$ARM_GCC_VER newlib-$NEWLIB_VER
rm -rf gmp-$GMP_VER mpfr-$MPFR_VER mpc-$MPC_VER

# Unpack everything.
tar xvJpf binutils-$BINUTILS_VER.tar.xz || exit 1
tar xvpf gcc-$SH_GCC_VER.tar.gz || exit 1
tar xvpf gcc-$ARM_GCC_VER.tar.gz || exit 1
tar xvpf newlib-$NEWLIB_VER.tar.gz || exit 1

# Unpack the GCC dependencies and move them into their required locations.
if [ -n "$GMP_VER" ]; then
    tar xvpf gmp-$GMP_VER.tar.gz || exit 1
    cp -apr gmp-$GMP_VER gcc-$SH_GCC_VER/gmp
    mv gmp-$GMP_VER gcc-$ARM_GCC_VER/gmp
fi

if [ -n "$MPFR_VER" ]; then
    tar xvpf mpfr-$MPFR_VER.tar.gz || exit 1
    cp -apr mpfr-$MPFR_VER gcc-$SH_GCC_VER/mpfr
    mv mpfr-$MPFR_VER gcc-$ARM_GCC_VER/mpfr
fi

if [ -n "$MPC_VER" ]; then
    tar xvpf mpc-$MPC_VER.tar.gz || exit 1
    cp -apr mpc-$MPC_VER gcc-$SH_GCC_VER/mpc
    mv mpc-$MPC_VER gcc-$ARM_GCC_VER/mpc
fi
