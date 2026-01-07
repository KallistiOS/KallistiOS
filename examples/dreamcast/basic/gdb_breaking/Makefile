# KallistiOS ##version##
#
# examples/dreamcast/basic/gdb_breaking/Makefile
# Copyright (c) 2025 Donald Haase
#

NAME = gdb_breaking
TARGET = $(NAME).elf
OBJS = $(NAME).o


include $(KOS_BASE)/Makefile.rules

all: rm-elf $(TARGET)

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
