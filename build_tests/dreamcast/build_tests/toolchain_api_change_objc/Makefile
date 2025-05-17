#
# Toolchain/API validator (ObjC version)
# Copyright (C) 2025 Donald Haase
#

TARGET = test.elf
OBJS = test.o

all: rm-elf $(TARGET)

#Include the dir with the patch to access it, as well as local for a stub <config.h>
KOS_CFLAGS += -I$(KOS_BASE)/utils/dc-chain/patches/gcc/ -I.

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -lobjc
