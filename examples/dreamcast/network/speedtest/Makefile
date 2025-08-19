#
# KallistiOS network/speedtest example
#
# Copyright (C) 2024 Andress Barajas
#

TARGET = speedtest.elf
OBJS = speedtest.o server.o handle_request.o romdisk.o
KOS_ROMDISK_DIR = romdisk
KOS_BUILD_SUBARCHS = pristine

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)
