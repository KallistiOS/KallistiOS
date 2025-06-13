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
LDLIBS = -lmruby

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

include $(KOS_BASE)/Makefile.rules

clean_extra:
	-rm -f $(BINARY) $(BOOTFILE) $(MRB_BYTECODE)

clean: clean_extra

$(MRB_BYTECODE): $(MRB_SOURCES)
	mrbc -g -B $(MRB_BYTECODE_IREP_NAME) -o $(MRB_BYTECODE) $(MRB_SOURCES)
