#
# Stack Protector Test/Example
# Copyright (C) 2021 Lawrence Sebald
# Copyright (C) 2024 Falco Girgis
#

TARGET = stackprotector.elf
OBJS = stackprotector.o
KOS_CFLAGS += -fstack-protector-all
KOS_GCCVER_MIN = 4.0.0

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


.PHONY: run dist clean rm-elf

