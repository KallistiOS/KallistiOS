#
# DC Clock App
# Based on Peter Hatch's Font Test
#
# Copyright (C) 2002 Megan Potter
# Copyright (C) 2026 Falco Girgis
#

TARGET = clock.elf
OBJS = clock.o romdisk.o
KOS_ROMDISK_DIR = romdisk
KOS_CPPFLAGS += -std=gnu++23
KOS_GCCVER_MIN = 13.0.0

include $(KOS_BASE)/Makefile.rules

ifeq ($(call KOS_GCCVER_MIN_CHECK,$(KOS_GCCVER_MIN)),1)

all: rm-elf $(TARGET)

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

else
  all $(TARGET) clean rm-elf run dist:
	$(KOS_GCCVER_MIN_WARNING)
endif
