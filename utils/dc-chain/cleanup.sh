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

# Clean up downloaded tarballs...
echo "Deleting downloaded packages..."
rm -f binutils-$BINUTILS_VER.tar.xz
rm -f gcc-$SH_GCC_VER.tar.gz
rm -f gcc-$ARM_GCC_VER.tar.gz
rm -f newlib-$NEWLIB_VER.tar.gz

if [ -n "$GMP_VER" ]; then
    rm -f gmp-$GMP_VER.tar.gz
fi

if [ -n "$MPFR_VER" ]; then
    rm -f mpfr-$MPFR_VER.tar.gz
fi

if [ -n "$MPC_VER" ]; then
    rm -f mpc-$MPC_VER.tar.gz
fi

echo "Done!"
echo "---------------------------------------"

# Clean up
echo "Deleting unpacked package sources..."
rm -rf binutils-$BINUTILS_VER
rm -rf gcc-$SH_GCC_VER
rm -rf gcc-$ARM_GCC_VER
rm -rf newlib-$NEWLIB_VER

if [ -n "$GMP_VER" ]; then
    rm -rf gmp-$GMP_VER
fi

if [ -n "$MPFR_VER" ]; then
    rm -rf mpfr-$MPFR_VER
fi

if [ -n "$MPC_VER" ]; then
    rm -rf mpc-$MPC_VER
fi

echo "Done!"
echo "---------------------------------------"

# Clean up any stale build directories.
echo "Cleaning up build directories..."

rm -rf build-*

echo "Done!"
echo "---------------------------------------"

# Clean up the logs.
echo "Cleaning up build logs..."

rm -f logs/*.log

echo "Done!"
