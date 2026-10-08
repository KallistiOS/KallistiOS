# KallistiOS environment variable settings. These are the shared pieces
# that are generated from the user config. Configure if you like.

export KOS_VERSION_MAJOR=`awk '$2 == "KOS_VERSION_MAJOR"{print $3; exit}' ${KOS_BASE}/include/kos/version.h`
export KOS_VERSION_MINOR=`awk '$2 == "KOS_VERSION_MINOR"{print $3; exit}' ${KOS_BASE}/include/kos/version.h`
export KOS_VERSION_PATCH=`awk '$2 == "KOS_VERSION_PATCH"{print $3; exit}' ${KOS_BASE}/include/kos/version.h`
export KOS_VERSION="${KOS_VERSION_MAJOR}.${KOS_VERSION_MINOR}.${KOS_VERSION_PATCH}"

# Default the kos-ports path if it isn't already set.
if [ -z "${KOS_PORTS}" ] ; then
    export KOS_PORTS="${KOS_BASE}/../kos-ports"
fi

# Arch kernel folder.
export KOS_ARCH_DIR="${KOS_BASE}/kernel/arch/${KOS_ARCH}"

# Add the compiler bins dir to the path if it is not already.
if ! expr ":$PATH:" : ".*:${KOS_CC_BASE}/bin:.*" > /dev/null ; then
  export PATH="${PATH}:${KOS_CC_BASE}/bin"
fi

# Add the build wrappers dir to the path if it is not already.
if ! expr ":$PATH:" : ".*:${KOS_BASE}/utils/build_wrappers:.*" > /dev/null ; then
  export PATH="${PATH}:${KOS_BASE}/utils/build_wrappers"
fi

# Our includes.
export KOS_INC_PATHS="${KOS_INC_PATHS} -isystem ${KOS_BASE}/include \
-isystem ${KOS_BASE}/kernel/arch/${KOS_ARCH}/include -isystem ${KOS_BASE}/addons/include/ \
-isystem ${KOS_PORTS}/include"

if [ "$KOS_TOOLCHAIN" == "gcc" ]; then
  . ${KOS_BASE}/env/environ_gcc.sh
elif [ "$KOS_TOOLCHAIN" == "clang" ]; then
  . ${KOS_BASE}/env/environ_clang.sh
else
  echo "ERROR: Unknown toolchain $KOS_TOOLCHAIN"
fi