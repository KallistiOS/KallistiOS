# KallistiOS ##version##
#
# examples/dreamcast/library/Makefile
# Copyright (C) 2024 Ruslan Rostovtsev
#

TARGET_NAME = library-test
TARGET = $(TARGET_NAME).elf
TARGET_LIB = lib$(TARGET_NAME).a
TARGET_BIN = $(TARGET_NAME).bin
OBJS = $(TARGET_NAME).o romdisk.o exports.o

EXPORTS_FILE = exports.txt
EXPORTS_SYMBOL = libtest_symtab
KOS_ROMDISK_DIR = romdisk

include $(KOS_BASE)/Makefile.rules

CPPFLAGS = -I./loadable-dependence

clean_loadable:
	-rm -rf $(TARGET_BIN) $(TARGET_LIB) romdisk.*
	cd loadable-dependence && make clean
	cd loadable-dependent && make clean

clean: clean_loadable

romdisk.img: loadable

loadable: $(TARGET_LIB)
	mkdir -p romdisk
	cd loadable-dependence && make && cp library-dependence.klf ../romdisk
	cd loadable-dependent && make && cp library-dependent.klf ../romdisk
