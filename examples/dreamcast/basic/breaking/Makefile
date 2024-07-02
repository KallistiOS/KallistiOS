# KallistiOS ##version##
#
# examples/dreamcast/basic/breaking/Makefile
# Copyright (c) 2024 Falco Girgis
#

TARGET = breaking.elf
OBJS = breaking.o
KOS_CFLAGS += -std=gnu2x -Wno-strict-aliasing
KOS_GCCVER_MIN = 13.0.0

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
