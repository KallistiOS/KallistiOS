#!/usr/bin/env bash
# KallistiOS Toolchain Builder (kos-chain)

# Install the KallistiOS headers that Newlib needs into the Newlib include
# directory of an existing toolchain. This is called by the 'fixup-newlib'
# Makefile target, but it can also be run on its own (e.g. after updating
# KallistiOS on MinGW/MSYS or MinGW-w64/MSYS2, where 'ln' can't be used).
#
# This is the only place where the list of fixed up headers is maintained.

usage() {
    cat <<EOF
Usage: $(basename "$0") [-c|-l] [-v] [kos_base] [newlib_inc]

Install the KallistiOS headers into the Newlib include directory.

  kos_base    KallistiOS root (containing 'include/'). Defaults to \$KOS_BASE.
  newlib_inc  Newlib include directory of the toolchain, e.g.
              /opt/toolchains/dc/sh-elf/sh-elf/include. It must exist.
              Defaults to \$NEWLIB_INC, then to
              \$KOS_CC_BASE/\$KOS_CC_PREFIX/include (from environ.sh).

Options:
  -c  Copy the 'kos' include directory (default on MinGW/MSYS and MSYS2).
  -l  Symlink the 'kos' include directory (default everywhere else).
  -v  Print each command as it is executed.
  -h  Display this help and exit.

Exits with a non-zero status if anything failed.
EOF
}

mode=
verbose=0
while getopts "clvh" opt; do
    case "$opt" in
        c) mode=copy ;;
        l) mode=link ;;
        v) verbose=1 ;;
        h) usage; exit 0 ;;
        *) usage >&2; exit 2 ;;
    esac
done
shift $((OPTIND - 1))

kos_base=${1:-$KOS_BASE}
newlib_inc=${2:-$NEWLIB_INC}
if [ -z "$newlib_inc" ] && [ -n "$KOS_CC_BASE" ] && [ -n "$KOS_CC_PREFIX" ]; then
    newlib_inc=$KOS_CC_BASE/$KOS_CC_PREFIX/include
fi

if [ -z "$kos_base" ] || [ -z "$newlib_inc" ]; then
    usage >&2
    exit 2
fi

if [ ! -d "$kos_base/include/kos" ]; then
    echo "$(basename "$0"): '$kos_base' is not a KallistiOS directory" >&2
    exit 1
fi

# Don't create a stray include directory if the path is wrong
if [ ! -d "$newlib_inc" ]; then
    echo "$(basename "$0"): '$newlib_inc' not found, is the toolchain installed?" >&2
    exit 1
fi

# Under MinGW/MSYS or MinGW-w64/MSYS2, the ln tool is not efficient, so it's
# better to do a simple copy. Please keep that in mind when upgrading
# KallistiOS or your toolchain!
if [ -z "$mode" ]; then
    case "$(uname -s)" in
        MINGW*|MSYS*) mode=copy ;;
        *) mode=link ;;
    esac
fi

set -e
if [ "$verbose" = "1" ]; then
    set -x
fi

mkdir -p "$newlib_inc/sys" "$newlib_inc/machine"

# KOS pthread.h is modified
# to define _POSIX_THREADS
# pthreads to kthreads mapping
cp "$kos_base/include/pthread.h" "$newlib_inc"
cp "$kos_base/include/sys/_pthreadtypes.h" "$newlib_inc/sys"
cp "$kos_base/include/sys/dirent.h" "$newlib_inc/sys"
# KOS machine/time.h defines _POSIX_TIMERS & co.
cp "$kos_base/include/machine/time.h" "$newlib_inc/machine"

# So KOS includes are available as kos/file.h. Any previous link or copy is
# removed first, so stale headers don't survive a KallistiOS update.
rm -rf "$newlib_inc/kos"
if [ "$mode" = "copy" ]; then
    cp -r "$kos_base/include/kos" "$newlib_inc"
else
    # The symlink must not be relative to the current directory
    ln -nsf "$(cd "$kos_base" && pwd)/include/kos" "$newlib_inc"
fi
