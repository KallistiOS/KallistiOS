# "System" libraries.
export KOS_LIB_PATHS="-L${KOS_BASE}/lib/${KOS_ARCH} -L${KOS_BASE}/addons/lib/${KOS_ARCH} -L${KOS_PORTS}/lib"
export KOS_LIBS="-Wl,--start-group -lkallisti -lm -lc -lcompiler-rt -Wl,--end-group"

# Main arch compiler paths.
export KOS_CC="${KOS_CC_BASE}/bin/clang"
export KOS_CCPLUS="${KOS_CC_BASE}/bin/clang++"
export KOS_AS="${KOS_CC_BASE}/bin/llvm-as"
export KOS_AR="${KOS_CC_BASE}/bin/llvm-ar"
export KOS_OBJCOPY="${KOS_CC_BASE}/bin/llvm-objcopy"
export KOS_OBJDUMP="${KOS_CC_BASE}/bin/llvm-objdump"
export KOS_ADDR2LINE="${KOS_CC_BASE}/bin/llvm-addr2line"
export KOS_GCOV="${KOS_CC_BASE}/bin/llvm-gcov"
export KOS_GPROF="${KOS_CC_BASE}/bin/llvm-gprof"
export KOS_LD="${KOS_CC_BASE}/bin/ld.lld"
export KOS_RANLIB="${KOS_CC_BASE}/bin/llvm-ranlib"
export KOS_STRIP="${KOS_CC_BASE}/bin/llvm-strip"
export KOS_SIZE="${KOS_CC_BASE}/bin/llvm-size"

export KOS_CFLAGS="${KOS_CFLAGS} ${KOS_INC_PATHS} -D_arch_${KOS_ARCH}=1 -D_arch_sub_${KOS_SUBARCH}=1 -Wall -g"
export KOS_CPPFLAGS="${KOS_CPPFLAGS} ${KOS_INC_PATHS_CPP}"

# Which standards modes we want to compile for.
# Note that this only covers KOS itself, not necessarily anything else compiled
# with kos-cc or kos-c++.
export KOS_CSTD="-std=gnu17"
export KOS_CPPSTD="-std=gnu++17"

export KOS_LDFLAGS="${KOS_CFLAGS} ${KOS_LDFLAGS} ${KOS_LD_SCRIPT} -nostdlib ${KOS_LIB_PATHS}"