# KallistiOS ##version##
#
# basic/threading/workqueue-safety/Makefile
# Copyright (C) 2026 Joseph Black
#

TARGET = workqueue-safety.elf
OBJS = workqueue-safety.o

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
