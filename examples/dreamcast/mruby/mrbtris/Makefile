# Mrbtris
# A sample Tetris clone for Sega Dreamcast written in Ruby
# Copyright (C) 2019-2024 Yuji Yokoo
# Copyright (C) 2020-2024 Mickaël "SiZiOUS" Cardoso

# Project Name
PROJECT = mrbtris

# Generated Programs
TARGET = $(PROJECT).elf
BINARY = $(PROJECT).bin
BOOTFILE = 1ST_READ.BIN

# Objects used in this project
# "$(PROJECT).o" is a transpiled C source file generated using MRB_BYTECODE
OBJS = $(PROJECT).o main.o dckos.o romdisk.o

# Libraries used by the project
LIBS = -lmruby

# ROM Disk directory used for resources
KOS_ROMDISK_DIR = romdisk

# Ruby sources
# The order here is important!
MRB_SOURCES = block_shapes.rb $(PROJECT).rb start.rb

# mruby script output (transpiled source from MRB_SOURCES)
MRB_BYTECODE = $(PROJECT).c

# Name of the compiled mruby script
# This should match the definition in "main.c"
MRB_BYTECODE_IREP_NAME = $(PROJECT)_bytecode

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET) $(BINARY) $(BOOTFILE) $(MRB_BYTECODE) romdisk.*

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) $(LIBS)

$(MRB_BYTECODE): $(MRB_SOURCES)
	mrbc -g -B $(MRB_BYTECODE_IREP_NAME) -o $(MRB_BYTECODE) $(MRB_SOURCES)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) romdisk.img
	$(KOS_BASE)/utils/elf2bin/elf2bin $(TARGET) $(BINARY) -f
	$(KOS_BASE)/utils/scramble/scramble $(BINARY) $(BOOTFILE)
