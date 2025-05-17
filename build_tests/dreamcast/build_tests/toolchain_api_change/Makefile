#
# Toolchain/API validator
# Copyright (C) 2025 Donald Haase
#

TARGET = test.elf
OBJS = test.o

all: rm-elf $(TARGET)

KOS_CFLAGS += -I$(KOS_BASE)/utils/dc-chain/patches/gcc/

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $@ $^
