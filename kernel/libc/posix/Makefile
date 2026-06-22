# KallistiOS ##version##
#
# kernel/libc/posix/Makefile
# Copyright (C) 2023, 2026 Falco Girgis
#

#
# This dir contains the implementation of POSIX-specific 
# C and C++ extensions which are implemented by KOS and 
# are not provided as part of Newlib.
#

CFLAGS += -std=gnu11
OBJS = clock_gettime.o posix_timer.o posix_memalign.o settimeofday.o sysconf.o

include $(KOS_BASE)/Makefile.prefab
