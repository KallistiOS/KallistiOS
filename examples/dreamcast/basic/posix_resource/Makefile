#
# POSIX resource example program
# Copyright (C) 2026 Donald Haase
#

NAME = resource
TARGET = $(NAME).elf
OBJS = $(NAME).o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $@ $^
