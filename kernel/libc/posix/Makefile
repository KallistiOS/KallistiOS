# KallistiOS ##version##
#
# kernel/libc/posix/Makefile
# Copyright (C) 2023 Falco Girgis
#

#
# This dir contains the implementation of POSIX-specific 
# C and C++ extensions which are implemented by KOS and 
# are not provided as part of Newlib.
#

CFLAGS += -std=gnu11
OBJS = posix_memalign.o clock_gettime.o settimeofday.o sysconf.o

ifneq ($(KOS_USE_TLSF),)
sysconf.o: CFLAGS += -DKOS_USE_TLSF
endif

include $(KOS_BASE)/Makefile.prefab
