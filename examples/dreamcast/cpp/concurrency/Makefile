#
# C++ Concurrency Test/Example
# Copyright (C) 2023, 2024 Falco Girgis
#

TARGET = concurrency.elf
OBJS = concurrency.o
KOS_CPPFLAGS += -std=c++20
KOS_GCCVER_MIN = 12.0.0

include $(KOS_BASE)/Makefile.rules

ifeq ($(call KOS_GCCVER_MIN_CHECK,$(KOS_GCCVER_MIN)),1)

all: rm-elf $(TARGET)

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

else
  all $(TARGET) clean rm-elf run dist:
	$(KOS_GCCVER_MIN_WARNING)
endif
