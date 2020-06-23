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

# Download everything.
if command -v curl >/dev/null 2>&1; then
    echo "Downloading Binutils $BINUTILS_VER..."
    curl --progress-bar -C - -O https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VER.tar.xz || exit 1
    echo "Downloading GCC $SH_GCC_VER..."
    curl --progress-bar -C - -O https://ftp.gnu.org/gnu/gcc/gcc-$SH_GCC_VER/gcc-$SH_GCC_VER.tar.gz || exit 1
    echo "Downloading GCC $ARM_GCC_VER..."
    curl --progress-bar -C - -O https://ftp.gnu.org/gnu/gcc/gcc-$ARM_GCC_VER/gcc-$ARM_GCC_VER.tar.gz || exit 1
    echo "Downloading Newlib $NEWLIB_VER..."
    curl --progress-bar -C - -O https://sourceware.org/pub/newlib/newlib-$NEWLIB_VER.tar.gz || exit 1

    if [ -n "$GMP_VER" ]; then
        echo "Downloading GMP $GMP_VER..."
        curl --progress-bar -C - -O https://gmplib.org/download/gmp/gmp-$GMP_VER.tar.gz || exit 1
    fi

    if [ -n "$MPFR_VER" ]; then
        echo "Downloading MPFR $MPFR_VER..."
        curl --progress-bar -C - -O https://www.mpfr.org/mpfr-current/mpfr-$MPFR_VER.tar.gz || exit 1
    fi

    if [ -n "$MPC_VER" ]; then
        echo "Downloading MPC $MPC_VER..."
        curl --progress-bar -C - -O https://ftp.gnu.org/gnu/mpc/mpc-$MPC_VER.tar.gz || exit 1
    fi
elif command -v wget >/dev/null 2>&1; then
    echo "Downloading binutils-$BINUTILS_VER..."
    wget -c https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VER.tar.xz || exit 1
    echo "Downloading GCC $SH_GCC_VER..."
    wget -c https://ftp.gnu.org/gnu/gcc/gcc-$SH_GCC_VER/gcc-$SH_GCC_VER.tar.gz || exit 1
    echo "Downloading GCC $ARM_GCC_VER..."
    wget -c https://ftp.gnu.org/gnu/gcc/gcc-$ARM_GCC_VER/gcc-$ARM_GCC_VER.tar.gz || exit 1
    echo "Downloading Newlib $NEWLIB_VER..."
    wget -c https://sourceware.org/pub/newlib/newlib-$NEWLIB_VER.tar.gz || exit 1

    if [ -n "$GMP_VER" ]; then
        echo "Downloading GMP $GMP_VER..."
        wget -c https://gmplib.org/download/gmp/gmp-$GMP_VER.tar.gz || exit 1
    fi

    if [ -n "$MPFR_VER" ]; then
        echo "Downloading MPFR $MPFR_VER..."
        wget -c 
	https://www.mpfr.org/mpfr-current/mpfr-$MPFR_VER.tar.gz || exit 1
    fi

    if [ -n "$MPC_VER" ]; then
        echo "Downloading MPC $MPC_VER..."
        wget -c https://ftp.gnu.org/gnu/mpc/mpc-$MPC_VER.tar.gz || exit 1
    fi
else
    echo >&2 "You must have either wget or cURL installed to use this script!"
    exit 1
fi
