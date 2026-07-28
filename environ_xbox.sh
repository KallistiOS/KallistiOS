# KallistiOS environment variable settings for the original Xbox platform.

# Use the retail Xbox as the default subarchitecture.
if [ -z "${KOS_SUBARCH}" ] ; then
    export KOS_SUBARCH="retail"
fi

# Generate 32-bit code for the Xbox's Pentium III-class CPU. Preserve frame
# pointers for KOS stack tracing and diagnostics.
export KOS_CFLAGS="${KOS_CFLAGS} \
-m32 \
-march=pentium3 \
-mtune=pentium3 \
-fno-pic \
-fno-pie \
-fno-omit-frame-pointer \
-ffunction-sections \
-fdata-sections \
-D__XBOX__"

# Make the assembler mode explicit.
export KOS_AFLAGS="${KOS_AFLAGS} --32"

# Link as 32-bit x86 and discard unused functions and data.
export KOS_LDFLAGS="${KOS_LDFLAGS} \
-m32 \
-Wl,--gc-sections"

# Xbox PE/COFF section layout.
export KOS_LD_SCRIPT="-T${KOS_BASE}/utils/ldscripts/xbox.ld"

# Architecture name understood by GDB.
export KOS_GDB_CPU="i386"
