# KallistiOS ##version##
#
# examples/dreamcast/basic/signal/Makefile
# Copyright (c) 2024 Falco Girgis
#

TARGET = signal.elf
OBJS = signal.o
KOS_CFLAGS += -std=gnu17

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -lm

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
