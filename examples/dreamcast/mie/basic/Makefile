# KallistiOS ##version##
#
# mie/basic/Makefile
# Copyright (C) 2026 Ruslan Rostovtsev
#

TARGET = mie_basic.elf
OBJS = mie_basic.o

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
	$(KOS_OBJCOPY) -R .stack -O binary $(TARGET) $(TARGET).bin
