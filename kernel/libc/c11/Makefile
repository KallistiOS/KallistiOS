# KallistiOS ##version##
#
# kernel/libc/c11/Makefile
# Copyright (C) 2014, 2015, 2023 Lawrence Sebald
#

KOS_CFLAGS += -std=gnu11 -D__KOS_LIBC

OBJS = call_once.o cnd_broadcast.o cnd_destroy.o cnd_init.o cnd_signal.o \
    cnd_timedwait.o cnd_wait.o mtx_destroy.o mtx_init.o mtx_lock.o \
    mtx_timedlock.o mtx_trylock.o mtx_unlock.o thrd_create.o thrd_current.o \
    thrd_detach.o thrd_equal.o thrd_exit.o thrd_join.o thrd_sleep.o \
    thrd_yield.o tss_create.o tss_delete.o tss_get.o tss_set.o \
    timespec_get.o timegm.o atomics.o

include $(KOS_BASE)/Makefile.prefab
