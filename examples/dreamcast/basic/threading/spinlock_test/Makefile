# KallistiOS ##version##
#
# basic/threading/spinlock_test/Makefile
#
# Copyright (C) 2023 Falco Girgis
#

TARGET = spinlock_test.elf
OBJS = spinlock_test.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS) 
	kos-cc -o $(TARGET) $(OBJS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
