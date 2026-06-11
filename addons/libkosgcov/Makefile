# KallistiOS ##version##
#
# addons/libkosgcov/Makefile
# Copyright (C) 2026 Andress Barajas
#

TARGET = libkosgcov.a
OBJS = kos_gcov.o
KOS_GCCVER_MIN = 12.0.0

.PHONY: libkosgcov

all: libkosgcov

include $(KOS_BASE)/addons/Makefile.prefab

ifeq ($(call KOS_GCCVER_MIN_CHECK,$(KOS_GCCVER_MIN)),1)

libkosgcov: defaultall

else

libkosgcov:
	$(KOS_GCCVER_MIN_WARNING)

endif
