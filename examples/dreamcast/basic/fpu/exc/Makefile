#
# FPU Exception Tests
# Copyright (C) 2002 Megan Potter
# Copyright (C) 2024 Falco Girgis
#

TARGET = fpu_exc.elf
OBJS = fpu_exc.o
KOS_GCCVER_MIN = 5.0.0



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
