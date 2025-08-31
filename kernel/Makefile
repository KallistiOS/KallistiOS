# KallistiOS ##version##
#
# kernel Makefile
# Copyright (C) 2001 Megan Potter
# Copyright (C) 2024 Falco Girgis
#

OBJS = version.o
SUBDIRS = arch debug fs thread net libc exports romdisk

# Everything from here up should be plain old C.
KOS_CFLAGS += $(KOS_CSTD)

all: defaultall
	rm -f $(KOS_BASE)/lib/$(KOS_ARCH)/libkallisti.a
	kos-ar rcs $(KOS_BASE)/lib/$(KOS_ARCH)/libromdiskbase.a romdisk/*.o
	kos-ar rcs $(KOS_BASE)/lib/$(KOS_ARCH)/libkallisti.a build/*.o

include $(KOS_BASE)/Makefile.prefab

clean: defaultclean
	rm -f build/*.o
	rm -f build/libc/*.o
	rm -f romdisk/*.o

run:
