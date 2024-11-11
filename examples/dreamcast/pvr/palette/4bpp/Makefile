#
# PVR Palette 4bpp example
# Copyright (C) 2011 Tvspelsfreak
# Copyright (C) 2024 Andress Barajas
#   

TARGET = 4bpp.elf
OBJS = 4bpp.o

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

