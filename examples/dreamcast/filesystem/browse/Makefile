#
# fs_* test program
#
# Copyright (C) 2024 Andress Barajas
#   

TARGET = browse.elf
OBJS = browse.o romdisk.o
KOS_ROMDISK_DIR = romdisk
LDLIBS = -lkosfat

include $(KOS_BASE)/Makefile.rules

stages:
	$(KOS_GENROMFS) -f stage1.img -d stage1 -v
	$(KOS_GENROMFS) -f stage2.img -d stage1 -v
