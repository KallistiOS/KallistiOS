#!/usr/bin/env bash

# Getting configuration from Makefile
source ./scripts/common.sh

print_banner "Cleaner"

while [ "$1" != "" ]; do
    PARAM=`echo $1 | awk -F= '{print $1}'`
    case $PARAM in
        --including-logs)
            DELETE_LOGS=1
            ;;
        --keep-downloads)
            KEEP_DOWNLOADS=1
            ;;
        *)
            echo "error: unknown parameter \"$PARAM\""
            exit 1
            ;;
    esac
    shift
done

function cleanup_dependency()
{
  local type="$1"
  local dep_name="$2"
  local dep_ver=$3_VER
  local dep_type=$3_TARBALL_TYPE

  local dep="${dep_name}-${!dep_ver}"

  if [ -n "${!dep_ver}" ]; then
    if [ "$type" == "package" ]; then
      rm -f "${dep}.tar.${!dep_type}"
	  else
	    rm -rf "${dep}"
    fi
  fi
}

function cleanup_dependencies()
{
  local type=$2

  if [ "$USE_CUSTOM_DEPENDENCIES" == "1" ]; then
    cleanup_dependency "$type" "GMP" "$1_GMP"
    cleanup_dependency "$type" "MPFR" "$1_MPFR"
    cleanup_dependency "$type" "MPC" "$1_MPC"
    cleanup_dependency "$type" "ISL" "$1_ISL"
  fi
}

if [ -z $KEEP_DOWNLOADS ]; then
  # Clean up downloaded tarballs...
  echo "Deleting downloaded packages..."

  rm -f binutils-$SH_BINUTILS_VER.tar.$SH_BINUTILS_TARBALL_TYPE \
        binutils-$ARM_BINUTILS_VER.tar.$ARM_BINUTILS_TARBALL_TYPE \
        gcc-$SH_GCC_VER.tar.$SH_GCC_TARBALL_TYPE \
        gcc-$ARM_GCC_VER.tar.$ARM_GCC_TARBALL_TYPE \
        newlib-$NEWLIB_VER.tar.$NEWLIB_TARBALL_TYPE

  if [ "$USE_CUSTOM_DEPENDENCIES" == "1" ]; then
    cleanup_dependencies "SH" "package"
    cleanup_dependencies "ARM" "package"
  fi

  if [ -f "gdb-$GDB_VER.tar.$GDB_TARBALL_TYPE" ]; then
    rm -f gdb-$GDB_VER.tar.$GDB_TARBALL_TYPE
  fi

  echo "Done!"
  echo "---------------------------------------"
fi

# Clean up unpacked sources...
echo "Deleting unpacked package sources..."
rm -rf binutils-$SH_BINUTILS_VER binutils-$ARM_BINUTILS_VER \
       gcc-$SH_GCC_VER gcc-$ARM_GCC_VER \
       newlib-$NEWLIB_VER \
       *.stamp	   

if [ "$USE_CUSTOM_DEPENDENCIES" == "1" ]; then
  cleanup_dependencies "SH" "dir"
  cleanup_dependencies "ARM" "dir"
fi

if [ -d "gdb-$GDB_VER" ]; then
  rm -rf gdb-$GDB_VER
fi

echo "Done!"
echo "---------------------------------------"

# Clean up any stale build directories.
echo "Cleaning up build directories..."

make="make"
if ! [ -z "$(command -v gmake)" ]; then
  make="gmake"
fi

# Cleaning up build directories.
${make} clean > /dev/null 2>&1

echo "Done!"
echo "---------------------------------------"

# Clean up the logs
if [ "$DELETE_LOGS" == "1" ]; then
  echo "Cleaning up build logs..."

  if [ -d "logs/" ]; then
    rm -f logs/*.log
    rmdir logs/
  fi

  echo "Done!"
  echo "---------------------------------------"
fi

if [ -z $KEEP_DOWNLOADS ]; then
  # Clean up config.guess
  echo "Cleaning up ${CONFIG_GUESS}..."

  if [ -f ${CONFIG_GUESS} ]; then
    rm -f "${CONFIG_GUESS}"
  fi

  echo "Done!"
fi
