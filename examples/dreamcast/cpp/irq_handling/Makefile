#
# C++ IRQ Handling Test/Example
# Copyright (C) 2024 Falco Girgis
#   

TARGET = irq_handling.elf
OBJS = irq_handling.o
KOS_CPPFLAGS += -std=gnu++26 
KOS_GCCVER_MIN = 14.1.0

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS) 

rm-elf:
	-rm -f $(TARGET) 

$(TARGET): $(OBJS) 
	kos-c++ -o $(TARGET) $(OBJS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)

