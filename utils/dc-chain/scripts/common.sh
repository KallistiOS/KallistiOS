#!/usr/bin/env bash

# This shell script extract versions from Makefile
# It's used in ./download.sh, ./unpack.sh and ./cleanup.sh

# See: https://stackoverflow.com/a/3352015/3726096
function trim()
{
  local var="$*"
  var="${var#"${var%%[![:space:]]*}"}"
  var="${var%"${var##*[![:space:]]}"}"
  printf '%s' "$var"
}

# Thanks to fedorqui 'SO stop harming'
# See: https://stackoverflow.com/a/39384347/3726096
function get_make_var()
{
  local token="$1"
  local file="${2:-config.mk}"
  local result=$(cat ${file} | grep "^[^#;]" | awk -v token="${token}" -F "=" '$0 ~ token {print $2}')
  echo "$(trim ${result})"
}

function tolower()
{
  echo "$1" | awk '{print tolower($0)}'
}

function toupper()
{
  echo "$1" | awk '{print toupper($0)}'
}

function print_banner()
{
  local var="$1"
  local banner_text=`get_make_var banner_text scripts/banner-variables.mk`
  printf "*** ${var} Utility for ${banner_text} ***\n\n" 
}

function setup_download_var()
{
  # Common Config Options
  export $1_DOWNLOAD_TYPE=`get_make_var $(tolower $1)_download_type`
  export $1_VER=`get_make_var $(tolower $1)_ver`
  
  # Git Repo Config Options
  export $1_GIT_REPO=`get_make_var $(tolower $1)_git_repo`
  export $1_GIT_BRANCH=`get_make_var $(tolower $1)_git_branch`

  # Fall back to older tarball_type if download_type was not specified
  local download_type="$1_DOWNLOAD_TYPE"
  if [ -z ${!download_type} ]; then
    echo $(tolower $1)_download_type not specified, falling back to $(tolower $1)_tarball_type
    export $1_DOWNLOAD_TYPE=`get_make_var $(tolower $1)_tarball_type`
  fi

  # If Git was not specified, assume tarball
  if [ "${!download_type}" != "git" ]; then
    export $1_TARBALL_TYPE="${!download_type}"
    export $1_DOWNLOAD_TYPE="tarball"
  fi
}

# Check for config.mk
if [ ! -f "config.mk" ]; then
  echo >&2 "The required config.mk file was not found!"
  exit 1    
fi
  
export CONFIG_GUESS="config.guess"

setup_download_var SH_BINUTILS
setup_download_var SH_GCC
setup_download_var NEWLIB
setup_download_var GDB
setup_download_var ARM_BINUTILS
setup_download_var ARM_GCC


# export GNU_MIRROR_URL=mirrors.kernel.org/gnu
export GNU_MIRROR_URL=ftpmirror.gnu.org
export SH_BINUTILS_TARBALL_URL=$GNU_MIRROR_URL/gnu/binutils/binutils-$SH_BINUTILS_VER.tar.$SH_BINUTILS_TARBALL_TYPE
export SH_GCC_TARBALL_URL=$GNU_MIRROR_URL/gnu/gcc/gcc-$SH_GCC_VER/gcc-$SH_GCC_VER.tar.$SH_GCC_TARBALL_TYPE
export NEWLIB_TARBALL_URL=sourceware.org/pub/newlib/newlib-$NEWLIB_VER.tar.$NEWLIB_TARBALL_TYPE
export GDB_TARBALL_URL=$GNU_MIRROR_URL/gnu/gdb/gdb-$GDB_VER.tar.$GDB_TARBALL_TYPE
export ARM_BINUTILS_TARBALL_URL=$GNU_MIRROR_URL/gnu/binutils/binutils-$ARM_BINUTILS_VER.tar.$ARM_BINUTILS_TARBALL_TYPE
export ARM_GCC_TARBALL_URL=$GNU_MIRROR_URL/gnu/gcc/gcc-$ARM_GCC_VER/gcc-$ARM_GCC_VER.tar.$ARM_GCC_TARBALL_TYPE


export DOWNLOAD_PROTOCOL="`get_make_var download_protocol`://"

export USE_CUSTOM_DEPENDENCIES=`get_make_var use_custom_dependencies`

setup_download_var SH_GMP
setup_download_var SH_MPFR
setup_download_var SH_MPC
setup_download_var SH_ISL
setup_download_var ARM_GMP
setup_download_var ARM_MPFR
setup_download_var ARM_MPC
setup_download_var ARM_ISL

export SH_GMP_TARBALL_URL=gcc.gnu.org/pub/gcc/infrastructure/gmp-${SH_GMP_VER}.tar.${SH_GMP_TARBALL_TYPE}
export SH_MPFR_TARBALL_URL=gcc.gnu.org/pub/gcc/infrastructure/mpfr-${SH_MPFR_VER}.tar.${SH_MPFR_TARBALL_TYPE}
export SH_MPC_TARBALL_URL=gcc.gnu.org/pub/gcc/infrastructure/mpc-${SH_MPC_VER}.tar.${SH_MPC_TARBALL_TYPE}
export SH_ISL_TARBALL_URL=gcc.gnu.org/pub/gcc/infrastructure/isl-${SH_ISL_VER}.tar.${SH_ISL_TARBALL_TYPE}
export ARM_GMP_TARBALL_URL=gcc.gnu.org/pub/gcc/infrastructure/gmp-${ARM_GMP_VER}.tar.${ARM_GMP_TARBALL_TYPE}
export ARM_MPFR_TARBALL_URL=gcc.gnu.org/pub/gcc/infrastructure/mpfr-${ARM_MPFR_VER}.tar.${ARM_MPFR_TARBALL_TYPE}
export ARM_MPC_TARBALL_URL=gcc.gnu.org/pub/gcc/infrastructure/mpc-${ARM_MPC_VER}.tar.${ARM_MPC_TARBALL_TYPE}
export ARM_ISL_TARBALL_URL=gcc.gnu.org/pub/gcc/infrastructure/isl-${ARM_ISL_VER}.tar.${ARM_ISL_TARBALL_TYPE}

# Retrieve the web downloader program available in this system.
export IS_CURL=0
export WEB_DOWNLOADER=
curl_cmd="curl --fail --location -C - -O"
wget_cmd="wget -c"
force_downloader=`get_make_var force_downloader`
if [ -z "$force_downloader" ]; then
  if command -v curl > /dev/null 2>&1; then
    export WEB_DOWNLOADER=${curl_cmd}
    export IS_CURL=1
  elif command -v wget > /dev/null 2>&1; then
    export WEB_DOWNLOADER=${wget_cmd}
  else
    echo >&2 "You must have either Wget or cURL installed!" || exit 1    
  fi
else
  if [ "$force_downloader" == "curl" ]; then
    export WEB_DOWNLOADER=${curl_cmd}
    export IS_CURL=1	
  elif [ "$force_downloader" == "wget" ]; then
    export WEB_DOWNLOADER=${wget_cmd}      
  else
    echo >&2 "Only Wget or cURL are supported!" || exit 1    
  fi
fi
