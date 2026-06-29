#
# pvr_rtt_sized
# Standalone small render-to-texture test for pvr_scene_begin_rtt().
# Copyright (C) 2026 Troy Davis
#

TARGET = pvr_rtt_sized.elf
OBJS = pvr_rtt_sized.o romdisk.o
KOS_ROMDISK_DIR = romdisk

KOS_CFLAGS += $(EXTRA_CFLAGS)
KOS_CPPFLAGS += -std=gnu++17 $(EXTRA_CFLAGS)

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS)
	kos-c++ -o $(TARGET) $(OBJS) -ldcplib

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)
