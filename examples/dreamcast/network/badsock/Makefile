#
# Sample failure with socket-based writes.
#
# Copyright (c) 2023 Donald Haase
#
# Built from: 
# KallistiOS network/httpd example
# Copyright (C) 2002 Megan Potter
#

# Put the filename of the output binary here
TARGET = badsock.elf

# List all of your C files here, but change the extension to ".o"
OBJS = main.o

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
