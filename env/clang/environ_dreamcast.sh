# KallistiOS environment variable settings. These are the shared pieces
# for the Dreamcast(tm) platform.

# Add the default subarch (DC) if one hasn't already been set.
if [ -z "${KOS_SUBARCH}" ] ; then
    export KOS_SUBARCH="pristine"
fi

# Add the default external DC tools path if it isn't already set.
if [ -z "${DC_TOOLS_BASE}" ] ; then
    export DC_TOOLS_BASE="${KOS_CC_BASE}/../bin"
fi

# Add the external DC tools dir to the path if it is not already.
if ! expr ":$PATH:" : ".*:${DC_TOOLS_BASE}:.*" > /dev/null ; then
  export PATH="${PATH}:${DC_TOOLS_BASE}"
fi

export KOS_CFLAGS="${KOS_CFLAGS} --target=shel-elf"
export KOS_AFLAGS="${KOS_AFLAGS} -little"
export KOS_LDFLAGS="${KOS_LDFLAGS} -Wl,--gc-sections"
export KOS_LD_SCRIPT="-T${KOS_BASE}/utils/ldscripts/shlelf.xc"

export KOS_GDB_CPU=sh4

if [ x${KOS_SUBARCH} = xnaomi ]; then
	export KOS_CFLAGS="${KOS_CFLAGS} -D__NAOMI__"
	export KOS_LDFLAGS="${KOS_LDFLAGS} -Wl,--defsym=LOAD_OFFSET=0x8c020000"
else
	export KOS_CFLAGS="${KOS_CFLAGS} -D__DREAMCAST__"
fi

# If we're building for DC, we need the ARM compiler paths as well.
if [ x${KOS_ARCH} = xdreamcast ]; then
	export DC_ARM_CC="${KOS_CC_BASE}/bin/clang"
	export DC_ARM_AS="${KOS_CC_BASE}/bin/llvm-as"
	export DC_ARM_AR="${KOS_CC_BASE}/bin/llvm-ar"
	export DC_ARM_OBJCOPY="${KOS_CC_BASE}/bin/llvm-objcopy"
	export DC_ARM_LD="${KOS_CC_BASE}/bin/ld.lld"
	export DC_ARM_CFLAGS="-mcpu=arm7di -Wall -O2 -fno-strict-aliasing -Wl,--fix-v4bx -Wa,--fix-v4bx"
	export DC_ARM_AFLAGS="-mcpu=arm7di --fix-v4bx"
	export DC_ARM_MAKE="make"
	export DC_ARM_START="${KOS_ARCH_DIR}/sound/arm/crt0.s"
	export DC_ARM_LDFLAGS="${DC_ARM_LDFLAGS} -Wl,-Ttext=0x00000000,-N -nostartfiles -nostdlib -e reset"
	export DC_ARM_LIB_PATHS=""
	export DC_ARM_LIBS="-Wl,--start-group -lgcc -Wl,--end-group"
fi
