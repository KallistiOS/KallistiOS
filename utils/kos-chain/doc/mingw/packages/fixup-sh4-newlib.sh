#!/usr/bin/env bash

# Paths come from the KallistiOS environment (environ.sh) when it has been
# sourced, otherwise the usual defaults are used. Change these if needed.
kos_base=${KOS_BASE:-/opt/toolchains/dc/kos}
newlib_inc=${KOS_CC_BASE:-/opt/toolchains/dc/sh-elf}/${KOS_CC_PREFIX:-sh-elf}/include

# In original kos-chain Makefile, 'kos' is a symbolic link. But the "ln" tool
# under MinGW/MSYS is not working properly, so each time you update
# KallistiOS, you'll need to run this tool to update the includes used by the
# SH4 compiler. The list of headers is maintained in kos-chain's
# 'scripts/fixup-newlib.sh'; this is just a convenience wrapper.
"$kos_base/utils/kos-chain/scripts/fixup-newlib.sh" "$kos_base" "$newlib_inc"
