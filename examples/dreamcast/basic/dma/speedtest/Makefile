#
# DMA speedtest program
# Copyright (C) 2025 Paul Cercueil
#

TARGET = speedtest.elf
OBJS = speedtest.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $@ $^
