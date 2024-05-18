# KallistiOS ##version##
#
# lua/basic/Makefile
# (c)2002 Megan Potter
#

TARGET = lua.elf
OBJS = lua.o

all: rm-elf $(TARGET)

KOS_CFLAGS += -I$(KOS_PORTS)/include/lua

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -llua

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
