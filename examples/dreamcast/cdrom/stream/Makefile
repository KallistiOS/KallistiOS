# KallistiOS ##version##
#
# examples/dreamcast/cd/stream/Makefile
# (c) 2025 Ruslan Rostovtsev
#

TARGET = cd-stream-test
OBJS = $(TARGET).o

all: rm-elf $(TARGET).elf

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET).elf $(TARGET).bin

$(TARGET).elf: $(OBJS)
	kos-cc -o $(TARGET).elf $(OBJS)

run: $(TARGET).elf
	$(KOS_LOADER) $(TARGET).elf

dist: $(TARGET).elf
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET).elf
	$(KOS_OBJCOPY) -R .stack -O binary $(TARGET).elf $(TARGET).bin
