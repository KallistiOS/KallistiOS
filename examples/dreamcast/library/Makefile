# KallistiOS ##version##
#
# examples/dreamcast/library/Makefile
# Copyright (C) 2024 Ruslan Rostovtsev
#

TARGET_NAME = library-test
TARGET = $(TARGET_NAME).elf
TARGET_LIB = lib$(TARGET_NAME).a
TARGET_BIN = $(TARGET_NAME).bin
OBJS = $(TARGET_NAME).o romdisk.o

EXPORTS_FILE = exports.txt
EXPORTS_SYMBOL = libtest_symtab
KOS_ROMDISK_DIR = romdisk

include $(KOS_BASE)/Makefile.rules

KOS_CFLAGS += -I./loadable-dependence

all: rm-elf $(TARGET_LIB) loadable $(TARGET)

clean: rm-elf
	-rm -f $(OBJS)
	-rm -rf ./romdisk
	cd loadable-dependence && make clean
	cd loadable-dependent && make clean

rm-elf:
	-rm -f $(TARGET) $(TARGET_BIN) $(TARGET_LIB) romdisk.*

loadable:
	mkdir -p romdisk
	cd loadable-dependence && make && cp library-dependence.klf ../romdisk
	cd loadable-dependent && make && cp library-dependent.klf ../romdisk

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)
	$(KOS_OBJCOPY) -R .stack -O binary $(TARGET) $(TARGET_BIN)
	$(KOS_SIZE) $(TARGET)
