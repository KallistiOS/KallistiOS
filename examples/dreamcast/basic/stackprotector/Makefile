#
# Stack Protector Test/Example
# Copyright (C) 2021 Lawrence Sebald
# Copyright (C) 2024 Falco Girgis
#

TARGET = stackprotector.elf
OBJS = stackprotector.o
KOS_CFLAGS += -fstack-protector-all
KOS_GCCVER_MIN = 4.0.0

include $(KOS_BASE)/Makefile.rules

ifeq ($(call KOS_GCCVER_MIN_CHECK,$(KOS_GCCVER_MIN)),1)

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

else
  all $(TARGET) clean rm-elf run dist:
	$(KOS_GCCVER_MIN_WARNING)
endif

.PHONY: run dist clean rm-elf

