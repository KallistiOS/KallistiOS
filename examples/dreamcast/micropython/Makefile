# KallistiOS ##version##
#
# examples/dreamcast/micropython/Makefile
# Copyright (C) 2023 Aaron Glazer
#

TARGET = micropython.elf

CFLAGS += -std=c99
CFLAGS += -I.
CFLAGS += -Wall -Og

SRCS += example.c
OBJS += example.o romdisk.o
KOS_ROMDISK_DIR = romdisk

# The rm-elf step is to remove the target before building, to force the
# re-creation of the rom disk.
all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -lmicropython

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)
