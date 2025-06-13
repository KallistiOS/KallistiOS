# KallistiOS ##version##
#
# examples/dreamcast/library/loadable-dependence/Makefile
# Copyright (C) 2024 Ruslan Rostovtsev
#

TARGET_NAME = library-dependence
TARGET = $(TARGET_NAME).klf
TARGET_LIB = lib$(TARGET_NAME).a
OBJS = $(TARGET_NAME).o exports.o

# For exporting kos_md5
LIBS = -lkosutils

# library-test exported stub for link test
DBG_LIBS = -llibrary-test

EXPORTS_SYMBOL = library_symtab
EXPORTS_FILE = exports.txt

CPPFLAGS = -I.
LDFLAGS = -L..

include $(KOS_BASE)/loadable/Makefile.prefab

all: $(TARGET_LIB)
