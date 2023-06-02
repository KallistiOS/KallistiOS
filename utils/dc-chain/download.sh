#!/usr/bin/env bash

# Getting configuration from Makefile
source ./scripts/common.sh || exit 1

print_banner "Downloader"

while [ "$1" != "" ]; do
    PARAM=`echo $1 | awk -F= '{print $1}'`
    case $PARAM in
        --config-guess-only)
            CONFIG_GUESS_ONLY=1
            ;;
        *)
            echo "error: unknown parameter \"$PARAM\""
            exit 1
            ;;
    esac
    shift
done

function download()
{
  local name=$1
  local prefix=$2
  
  local ver="$2_VER"
  local download_type="$2_DOWNLOAD_TYPE"

  # if version is not empty
  if [ ! -n "${!ver}" ]; then
    echo "ERROR: ${name} version must not be empty"
    exit 1
  fi

  # source tarball
  if [[ "${!download_type}" == "tarball" ]]; then
    local url=$2_TARBALL_URL
    local filename=$(basename "${!url}")
      if [ -f $filename ]; then
        echo "${name} ${!ver} was already downloaded"     
      else
        echo "Downloading ${name} ${!ver} from ${!url}"
        ${WEB_DOWNLOADER} "${DOWNLOAD_PROTOCOL}${!url}" || exit 1
      fi
  # source tarball
  elif [[ "${!download_type}" == "git" ]]; then
    local filename=$(tolower $name)-${!ver}
    local repo=$2_GIT_REPO
    local branch=$2_GIT_BRANCH
    if [ -d $filename ]; then
      echo "${name} \"${filename}\" already exists"
    else
      if [ -z ${!branch} ]; then
        echo "Downloading ${name} from ${!repo}"
        ${GIT_CLONE} ${!repo} ${filename} || exit 1
      else
        echo "Downloading ${name} from ${!repo}:${!branch}"
        ${GIT_CLONE} ${!repo} -b ${!branch} ${filename} || exit 1
      fi
    fi

  # invalid type (shouldn't get here)
  else
    printf "Invalid Download Type ${!download_type}\n"
    exit 1
  fi
}

function download_dependencies()
{
  if [ "$USE_CUSTOM_DEPENDENCIES" == "1" ]; then
    download "GMP"  "$1_GMP"
    download "MPFR" "$1_MPFR"
    download "MPC"  "$1_MPC"
    download "ISL"  "$1_ISL"
  fi
}

GIT_CLONE="git clone --depth=1 --single-branch"

# Download everything.
if [ -z "${CONFIG_GUESS_ONLY}" ]; then
  # Downloading SH components
  download "Binutils" "SH_BINUTILS"
  download "GCC" "SH_GCC"
  download_dependencies "SH"
  download "Newlib" "NEWLIB"
  
  # Downloading ARM components
  download "Binutils" "ARM_BINUTILS"
  download "GCC" "ARM_GCC"
  download_dependencies "ARM"
fi

# Downloading config.guess.
if [ ! -f ${CONFIG_GUESS} ]; then
  WEB_DOWNLOAD_OUTPUT_SWITCH="-O"
  if [ ! -z "${IS_CURL}" ] && [ "${IS_CURL}" != "0" ]; then
    WEB_DOWNLOADER="$(echo "${WEB_DOWNLOADER}" | sed '-es/-O//')"
    WEB_DOWNLOAD_OUTPUT_SWITCH="-o"
  fi

  echo "Downloading ${CONFIG_GUESS}..."
  ${WEB_DOWNLOADER} ${WEB_DOWNLOAD_OUTPUT_SWITCH} ${CONFIG_GUESS} "http://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=${CONFIG_GUESS};hb=HEAD" || exit 1

  # This is needed for all systems except MinGW.
  chmod +x "./${CONFIG_GUESS}"
fi

echo "Done!"
