#!/bin/sh

# These version numbers are all that should ever have to be changed.
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

# Download everything.
if command -v curl >/dev/null 2>&1; then
    echo "Downloading Binutils $BINUTILS_VER..."
    curl --progress-bar -C - -O $BINUTILS_URL || exit 1
    echo "Downloading GCC $SH_GCC_VER..."
    curl --progress-bar -C - -O $SH_GCC_URL || exit 1
    echo "Downloading GCC $ARM_GCC_VER..."
    curl --progress-bar -C - -O $ARM_GCC_URL || exit 1
    echo "Downloading Newlib $NEWLIB_VER..."
    curl --progress-bar -C - -O $NEWLIB_URL || exit 1

    if [ -n "$GMP_VER" ]; then
        echo "Downloading GMP $GMP_VER..."
        curl --progress-bar -C - -O $GMP_URL || exit 1
    fi

    if [ -n "$MPFR_VER" ]; then
        echo "Downloading MPFR $MPFR_VER..."
        curl --progress-bar -C - -O $MPFR_URL || exit 1
    fi

    if [ -n "$MPC_VER" ]; then
        echo "Downloading MPC $MPC_VER..."
        curl --progress-bar -C - -O $MPC_URL || exit 1
    fi
elif command -v wget >/dev/null 2>&1; then
    echo "Downloading binutils-$BINUTILS_VER..."
    wget -c $BINUTILS_URL || exit 1
    echo "Downloading GCC $SH_GCC_VER..."
    wget -c $SH_GCC_URL || exit 1
    echo "Downloading GCC $ARM_GCC_VER..."
    wget -c $ARM_GCC_URL || exit 1
    echo "Downloading Newlib $NEWLIB_VER..."
    wget -c $NEWLIB_URL || exit 1

    if [ -n "$GMP_VER" ]; then
        echo "Downloading GMP $GMP_VER..."
        wget -c $GMP_URL || exit 1
    fi

    if [ -n "$MPFR_VER" ]; then
        echo "Downloading MPFR $MPFR_VER..."
        wget -c $MPFR_URL || exit 1
    fi

    if [ -n "$MPC_VER" ]; then
        echo "Downloading MPC $MPC_VER..."
        wget -c $MPC_URL || exit 1
    fi
else
    echo >&2 "You must have either wget or cURL installed to use this script!"
    exit 1
fi
