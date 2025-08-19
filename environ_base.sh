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
export KOS_INC_PATHS="${KOS_INC_PATHS} -I${KOS_BASE}/include \
-I${KOS_BASE}/kernel/arch/${KOS_ARCH}/include -I${KOS_BASE}/addons/include/ -I${KOS_BASE}/addons/include/${KOS_ARCH} \
-I${KOS_PORTS}/include"

# "System" libraries.
export KOS_LIB_PATHS="-L${KOS_BASE}/lib/${KOS_ARCH} -L${KOS_BASE}/addons/lib/${KOS_ARCH} -L${KOS_PORTS}/lib"
export KOS_LIBS="-Wl,--start-group -lkallisti -lm -lc -lgcc -Wl,--end-group"

# Main arch compiler paths.
export KOS_CC="${KOS_CC_BASE}/bin/${KOS_CC_PREFIX}-gcc"
export KOS_CCPLUS="${KOS_CC_BASE}/bin/${KOS_CC_PREFIX}-g++"
export KOS_AS="${KOS_CC_BASE}/bin/${KOS_CC_PREFIX}-as"
export KOS_AR="${KOS_CC_BASE}/bin/${KOS_CC_PREFIX}-gcc-ar"
export KOS_OBJCOPY="${KOS_CC_BASE}/bin/${KOS_CC_PREFIX}-objcopy"
export KOS_OBJDUMP="${KOS_CC_BASE}/bin/${KOS_CC_PREFIX}-objdump"
export KOS_ADDR2LINE="${KOS_CC_BASE}/bin/${KOS_CC_PREFIX}-addr2line"
export KOS_LD="${KOS_CC_BASE}/bin/${KOS_CC_PREFIX}-ld"
export KOS_RANLIB="${KOS_CC_BASE}/bin/${KOS_CC_PREFIX}-gcc-ranlib"
export KOS_STRIP="${KOS_CC_BASE}/bin/${KOS_CC_PREFIX}-strip"
export KOS_SIZE="${KOS_CC_BASE}/bin/${KOS_CC_PREFIX}-size"

# Pull in the arch environ file.
. ${KOS_BASE}/environ_${KOS_ARCH}.sh

export KOS_CFLAGS="${KOS_CFLAGS} ${KOS_INC_PATHS} -D_arch_${KOS_ARCH} -D_arch_sub_${KOS_SUBARCH} -Wall -g"
export KOS_CPPFLAGS="${KOS_CPPFLAGS} ${KOS_INC_PATHS_CPP}"

# Which standards modes we want to compile for.
# Note that this only covers KOS itself, not necessarily anything else compiled
# with kos-cc or kos-c++.
export KOS_CSTD="-std=gnu17"
export KOS_CPPSTD="-std=gnu++17"

export KOS_GCCVER="`kos-cc -dumpversion`"

export KOS_LDFLAGS="${KOS_CFLAGS} ${KOS_LDFLAGS} ${KOS_LD_SCRIPT} -nostdlib ${KOS_LIB_PATHS}"
