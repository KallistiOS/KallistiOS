#
# 2D_Textured_Quad_PNG Makefile
#
# Author: Jason Rost (OniEnzeru)
# Date:	  1/22/2024
#

KOS_LOADER = dc-tool-ip

# Directories
KOS_ROMDISK_DIR := romdisk

# File aggregators
SRCS		:= main.cpp
OBJS		:= main.o romdisk.o

# Compiler Flags
CXXFLAGS	:= -std=c++20
LDLIBS 		:= -lGL -lkmg -lkosutils -lm

TARGET = 2D_Textured_Quad_PNG.elf

.PHONY: all clean rm-elf push

all: clean $(TARGET)
include $(KOS_BASE)/Makefile.rules

clean:
	rm -Rf $(OBJS) $(TARGET) romdisk.*

$(TARGET): $(OBJS)
	kos-c++ $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS)

$(OBJ)/%.o: $(SRC)/%.cpp | $(OBJ)
	kos-c++ $(CXXFLAGS) -c $< -o $@

push: $(TARGET)
	$(KOS_LOADER) -x $(TARGET) -t 10.10.10.3 #ip of dc-load
