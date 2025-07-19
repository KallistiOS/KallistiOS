# KallistiOS ##version##
#
# Copyright (C) 2025 Troy Davis
#
# examples/dreamcast/keyboard/keytest/Makefile
#

TARGET = keyrawtest.elf
OBJS = keyrawtest.o

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
