# KallistiOS environment variable settings. These are the shared pieces
# for host platform building.

# Add the default subarch (DC) if one hasn't already been set.
if [ -z "${KOS_SUBARCH}" ] ; then
    export KOS_SUBARCH="pristine"
fi

export KOS_INC_PATHS="${KOS_INC_PATHS} -I${KOS_CC_BASE}/include/newlib"
export KOS_AFLAGS="${KOS_AFLAGS} -little"

export KOS_LDFLAGS="${KOS_LDFLAGS} ${KOS_SH4_PRECISION} -ml -Wl,--gc-sections"
export KOS_LD_SCRIPT="-T${KOS_BASE}/utils/ldscripts/shlelf.xc"

export KOS_GDB_CPU=sh4
