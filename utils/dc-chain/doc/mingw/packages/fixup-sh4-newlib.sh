#!/usr/bin/env bash

# You can change these options if needed.
export toolchains_base=/opt/toolchains/dc
export kos_base=$toolchains_base/kos
export newlib_inc=$toolchains_base/sh-elf/sh-elf/include

# In original dc-chain Makefile, the instructions below are symbolic links.
# But the "ln" tool under MinGW/MSYS is not working properly.
# So each time you update KallistiOS, you'll need to run this tool to update
# the includes used by the SH4 compiler.
cp -r $kos_base/include/kos $newlib_inc
cp -r $kos_base/kernel/arch/dreamcast/include/arch $newlib_inc
cp -r $kos_base/kernel/arch/dreamcast/include/dc $newlib_inc

# These instructions are just fail-safes, as in the original dc-chain Makefile
# they are already "cp" instructions.
cp $kos_base/include/pthread.h $newlib_inc
cp $kos_base/include/sys/_pthread.h $newlib_inc/sys
cp $kos_base/include/sys/sched.h $newlib_inc/sys
