#!/usr/bin/env bash

# Getting versions defined in Makefile
source ./version.sh

export config_guess="./config.guess"

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
rm -f gcc-$GCC_VER.tar.bz2
rm -f newlib-$NEWLIB_VER.tar.gz

if [ -n "$GMP_VER" ]; then
    rm -f gmp-$GMP_VER.tar.bz2
fi

if [ -n "$MPFR_VER" ]; then
    rm -f mpfr-$MPFR_VER.tar.bz2
fi

if [ -n "$MPC_VER" ]; then
    rm -f mpc-$MPC_VER.tar.gz
fi

if [ -f "gdb-$GDB_VER.tar.gz" ]; then
	rm -f gdb-$GDB_VER.tar.gz
fi

if [ -f "insight-${INSIGHT_VER}a.tar.bz2" ]; then
	rm -f insight-${INSIGHT_VER}a.tar.bz2
fi

echo "Done!"
echo "---------------------------------------"

# Clean up unpacked sources...
echo "Deleting unpacked package sources..."
rm -rf binutils-$BINUTILS_VER
rm -rf gcc-$GCC_VER
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

if [ -d "gdb-$GDB_VER" ]; then
    rm -rf gdb-$GDB_VER
fi

if [ -d "insight-$INSIGHT_VER" ]; then
    rm -rf insight-$INSIGHT_VER
fi

echo "Done!"
echo "---------------------------------------"

# Clean up any stale build directories.
echo "Cleaning up build directories..."

export make="make"
if ! [ -z "command -v gmake" ]; then
	export make="gmake"
fi

# Cleaning up build directories.
${make} clean

echo "Done!"
echo "---------------------------------------"

# Clean up the logs
echo "Cleaning up build logs..."

if [ -d "logs/" ]; then
	rm -f logs/*.log
	rmdir logs/
fi

echo "Done!"
echo "---------------------------------------"

# Clean up config.guess
echo "Cleaning up ${config_guess}..."

if [ -f ${config_guess} ]; then
	rm -f ${config_guess}
fi

echo "Done!"