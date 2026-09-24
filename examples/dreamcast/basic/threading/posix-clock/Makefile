# KallistiOS ##version##
TARGET = posix-clock.elf
OBJS = posix-clock.o

all: $(TARGET)

include $(KOS_BASE)/Makefile.rules

posix-clock.o: $(KOS_BASE)/utils/posix-clock-test/contract.h

$(TARGET): $(OBJS) $(KOS_BASE)/lib/dreamcast/libkallisti.a
	kos-cc -o $(TARGET) $(OBJS)

clean:
	-rm -f $(TARGET) $(OBJS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)
