#
# KallistiOS ##version##
#
# examples/dreamcast/gldc/2D_Textured_Quad_Example-GLDC/Makefile
# Copyright (C) 2024 Jason Rost (OniEnzeru)
#

KOS_LOADER = dc-tool-ip

# Directories
KOS_ROMDISK_DIR := romdisk

# File aggregators
SRCS		:= main.cpp
OBJS		:= main.o romdisk.o

# Compiler Flags
CXXFLAGS	:= -std=c++20
LDLIBS 		:= -lstb_image -lGL -lkmg -lkosutils -lm

TARGET = 2D_Textured_Quad_PNG.elf

.PHONY: all clean push

all: rm-elf $(TARGET)
include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -rf $(OBJS) romdisk.*

$(TARGET): $(OBJS)
	kos-c++ $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS)

$(OBJ)/%.o: $(SRC)/%.cpp | $(OBJ)
	kos-c++ $(CXXFLAGS) -c $< -o $@

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)

rm-elf:
	-rm -f $(TARGET)

run: $(TARGET)
	$(KOS_LOADER) -x $(TARGET)
