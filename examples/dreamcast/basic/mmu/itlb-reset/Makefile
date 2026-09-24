# KallistiOS ##version##
# Copyright (C) 2026 Joseph Black
TARGET = itlb-reset.elf
OBJS = itlb-reset.o tlb-array.o
all: $(TARGET)
include $(KOS_BASE)/Makefile.rules
$(TARGET): $(OBJS)
	kos-cc -o $@ $(OBJS)
clean:
	$(RM) $(TARGET) $(OBJS)
run: $(TARGET)
	$(KOS_LOADER) $(TARGET)
