# KallistiOS ##version##
#
# examples/dreamcast/basic/waiting/Makefile
# Copyright (c) 2024 Falco Girgis
#

TARGET = waiting.elf
OBJS = waiting.o
KOS_CFLAGS += -std=gnu2x

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
