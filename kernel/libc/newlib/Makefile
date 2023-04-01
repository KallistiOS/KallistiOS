# KallistiOS ##version##
#
# kernel/newlib/Makefile
# Copyright (C)2004 Megan Potter
#

#
# This dir contains Newlib shims required to get Newlib to link and work
# properly. You can compile Newlib with -DREENTRANT_SYSCALLS_PROVIDED
# or without it, there are versions of both here to take care of that.
# The Newlib guys call this "namespace impure" but at least it works
# without needing recompilation ;)
#

OBJS = lock_common.o newlib_close.o newlib_env_lock.o newlib_environ.o \
	newlib_execve.o newlib_exit.o \
	newlib_fork.o newlib_fstat.o newlib_getentropy.o newlib_getpid.o \
	newlib_gettimeofday.o newlib_isatty.o newlib_kill.o newlib_link.o \
	newlib_lseek.o newlib_malloc.o newlib_open.o \
	newlib_read.o newlib_sbrk.o newlib_stat.o newlib_times.o \
	newlib_unlink.o newlib_wait.o newlib_write.o newlib_fcntl.o \
	verify_newlib.o

include $(KOS_BASE)/Makefile.prefab
