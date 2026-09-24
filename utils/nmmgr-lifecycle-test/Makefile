# KallistiOS ##version##
#
# Host-side name-manager lifecycle tests.
# Copyright (C) 2026 Joseph Black

CC ?= cc
CFLAGS ?= -O2
HOST_CSTD ?= gnu17
CFLAGS += -std=$(HOST_CSTD) -Wall -Wextra -Werror -pthread
ROOT := ../..
INCLUDES := -Iinclude
SOURCE := $(ROOT)/kernel/exports/nmmgr.c $(ROOT)/kernel/exports/exports.c
TARGET := nmmgr-lifecycle-test

.PHONY: all clean test

all: test

$(TARGET): nmmgr-lifecycle-test.c $(SOURCE) $(ROOT)/include/kos/nmmgr.h \
           $(ROOT)/include/kos/exports.h include/kos/exports.h
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ nmmgr-lifecycle-test.c $(SOURCE)

test: $(TARGET)
	./$(TARGET)

clean:
	$(RM) $(TARGET)
