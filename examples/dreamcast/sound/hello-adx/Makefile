#
# LibADX Test
# Copyright (C) 2011-2013 Josh 'PH3NOM' Pearson <ph3nom.dcmc@gmail.com>
# Copyright (C) 2024 The KOS Team and contributors
#

TARGET = libADXplay.elf
OBJS = libADXplay.o romdisk.o
KOS_ROMDISK_DIR = romdisk

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -lADX

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)

