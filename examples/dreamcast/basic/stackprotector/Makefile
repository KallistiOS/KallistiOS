#
# Stack Protector Test/Example
# Copyright (C) 2021 Lawrence Sebald
# Copyright (C) 2024 Falco Girgis
#

TARGET = stackprotector.elf
OBJS = stackprotector.o
# Disable optimization that might avoid the overflow and the warning the intentional error would create
KOS_CFLAGS += -O0 -Wno-stringop-overflow -Wno-analyzer-out-of-bounds
# Force stack protector on
KOS_CFLAGS += -fstack-protector-all

include $(KOS_BASE)/Makefile.rules

all: rm-elf $(TARGET)

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
