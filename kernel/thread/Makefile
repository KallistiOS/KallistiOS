# KallistiOS ##version##
#
# thread/Makefile
# Copyright (C) 2001 Megan Potter
#

OBJS =  sem.o cond.o mutex.o genwait.o
OBJS += thread.o rwsem.o once.o tls.o barrier.o
OBJS += oneshot_timer.o worker.o
SUBDIRS = 

# On toolchains that support the C23 standard (aka. GCC > 14), compile-test
# our gthr-kos.h against the regular KOS includes, to make sure that the
# toolchain file does not lag behind.
OBJS += $(intcmp $(firstword $(subst ., ,${KOS_GCCVER})),14,,,toolchain_test.o)

include $(KOS_BASE)/Makefile.prefab

toolchain_test.o: ${KOS_BASE}/utils/dc-chain/patches/gcc/gthr-kos.h
	kos-cc -DTEST_GTHR_KOS_API -std=c23 -c $< -o $@
