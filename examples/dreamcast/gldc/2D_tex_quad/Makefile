#
# KallistiOS ##version##
#
# examples/dreamcast/gldc/2D_tex_quad/Makefile
# Copyright (C) 2024 Jason Rost (OniEnzeru)
#


# Directories
KOS_ROMDISK_DIR := romdisk

# File aggregators
SRCS		:= main.cpp
OBJS		:= main.o romdisk.o

# Compiler Flags
KOS_CPPSTD	:= -std=c++20
LDLIBS 		:= -lstb_image -lGL -lkmg -lkosutils

TARGET = 2D_tex_quad.elf

.PHONY: all clean push

all: rm-elf $(TARGET)
include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -rf $(OBJS) romdisk.*

$(TARGET): $(OBJS)
	kos-c++ -o $(TARGET) $(OBJS) $(LDLIBS)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)

rm-elf:
	-rm -f $(TARGET)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)
