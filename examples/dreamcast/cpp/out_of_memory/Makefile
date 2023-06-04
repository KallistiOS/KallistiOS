#
# C++ out-of-memory demonstration
# (c)2023 Falco Girgis
#

TARGET = out_of_memory.elf
OBJS = out_of_memory.o

KOS_CPPFLAGS += -fexceptions -std=c++11

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-c++ -o $(TARGET) $(OBJS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
