# KallistiOS ##version##
#
# video/screenshot/Makefile
# Copyright (C) 2024 Andy Barajas
#

TARGET = screenshot.elf
OBJS = screenshot.o

# For Mac, Linux, Ubuntu sudo is required with the -c "." command to save data to the PC
#    sudo $(KOS_LOADER) $(TARGET) -c "."
KOS_LOADER_FLAGS = -c "."

include $(KOS_BASE)/Makefile.rules

clean_screenshots:
	-rm -f screenshot*.ppm

clean: clean_screenshots
