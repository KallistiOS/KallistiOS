# KallistiOS ##version##
#
# examples/dreamcast/micropython/Makefile
# Copyright (C) 2023 Aaron Glazer
#

TARGET = micropython.elf
OBJS += example.o romdisk.o
KOS_ROMDISK_DIR = romdisk

CFLAGS = -std=c99 -Wall -Og
CPPFLAGS = -I.
LDLIBS = -lmicropython

include $(KOS_BASE)/Makefile.rules
