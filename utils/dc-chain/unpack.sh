#!/usr/bin/env bash

# Getting configuration from Makefile
source ./scripts/common.sh

print_banner "Unpacker"

function unpack()
{
  local name="$1"
  local ver=$2_VER
  local ext=$2_TARBALL_TYPE
  local download_type=$2_DOWNLOAD_TYPE

  local dirname=$(tolower $name)-${!ver}
  local filename=$dirname.tar.${!ext}

  if [[ "${!download_type}" == "tarball" ]]; then
    echo "Tarball $name"
    # Remove old directory
    rm -rf $dirname

    if [ ! -f "$filename" ]; then
      echo "Required file not found: $filename"
      exit 1
    fi
  
    if [ ! -d "$dirname" ]; then
      echo "Unpacking $name ${!ver}..."
      tar xf "$filename" || exit 1
    fi
  fi

  
}

function unpack_dependency()
{
  local gcc_ver="$1"
  local dep_name="$2"  
  local dep_ver=$2_VER
  local dep_ext=$2_TARBALL_TYPE

  local download_type=$(tolower "$dep_name")_DOWNLOAD_TYPE
  local path=$(tolower "$dep_name")

  if [[ "${!download_type}" == "tarball" ]]; then
    if [ -n "${!dep_ver}" ]; then
      echo "  Unpacking $dep_name ${!dep_ver}..."
      tar xf $path-${!dep_ver}.tar.${!dep_ext} || exit 1
      mv $path-${!dep_ver} gcc-$gcc_ver/$path
    fi
  fi
}

function unpack_dependencies()
{
  local gcc_ver=$1_GCC_VER
  
  echo "Unpacking prerequisites for GCC ${!gcc_ver}..."

  if [ "$USE_CUSTOM_DEPENDENCIES" == "1" ]; then
    unpack_dependency "${!gcc_ver}" "$1_GMP"
    unpack_dependency "${!gcc_ver}" "$1_MPFR"
    unpack_dependency "${!gcc_ver}" "$1_MPC"
    unpack_dependency "${!gcc_ver}" "$1_ISL"
  else
    cd ./gcc-${!gcc_ver} && ./contrib/download_prerequisites && cd ..
  fi
}


echo "Preparing unpacking..."

# Unpack SH components
unpack "Binutils" "SH_BINUTILS"
unpack "GCC" "SH_GCC"
unpack_dependencies "SH"
unpack "Newlib" "NEWLIB"


# Unpack ARM components
unpack "Binutils" "ARM_BINUTILS"
unpack "GCC" "ARM_GCC"
unpack_dependencies "ARM"

echo "Done!"
