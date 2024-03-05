# KallistiOS environment variable settings. These are the shared pieces
# for the GBA(tm) platform.

# This is currently configured to produce only Thumb code
export KOS_CFLAGS="${KOS_CFLAGS} -mcpu=arm7tdmi -mtune=arm7tdmi -mthumb -mthumb-interwork -ffreestanding"
export KOS_AFLAGS="${KOS_AFLAGS} "
export KOS_LDFLAGS="${KOS_LDFLAGS} -g -mthumb -mthumb-interwork -Wl,-Ttext=0x08000000,-Tdata=0x02000000"

export KOS_LIB_PATHS="${KOS_LIB_PATHS} -L${DEVKITARM}/lib/gcc/${KOS_CC_PREFIX} -L${DEVKITARM}/${KOS_CC_PREFIX} -L${DEVKITARM}/lib/thumb"

export KOS_LD_SCRIPT="-T${KOS_BASE}/kernel/arch/gba/gba.ld.script"

