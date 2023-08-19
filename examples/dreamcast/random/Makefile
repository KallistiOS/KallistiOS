#
# Basic KallistiOS skeleton / test program
# Copyright (C)2001-2004 Megan Potter
#

# Put the filename of the output binary here
TARGET = random.elf

# List all of your C files here, but change the extension to ".o"
OBJS = random.o

KOS_CFLAGS += -std=c99

# The rm-elf step is to remove the target before building, to force the
# re-creation of the rom disk.
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
