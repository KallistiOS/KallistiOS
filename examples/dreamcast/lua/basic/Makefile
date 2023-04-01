# KallistiOS ##version##
#
# lua/basic/Makefile
# (c)2002 Megan Potter
#

all: rm-elf lua.elf

KOS_CFLAGS += -I$(KOS_PORTS)/include/lua

include $(KOS_BASE)/Makefile.rules

OBJS = lua.o

clean:
	-rm -f lua.elf $(OBJS)

rm-elf:
	-rm -f lua.elf

lua.elf: $(OBJS)
	kos-cc -o lua.elf $(OBJS) $(DATAOBJS) $(OBJEXTRA) -llua -lm

run: lua.elf
	$(KOS_LOADER) lua.elf

dist:
	-rm -f $(OBJS)
	$(KOS_STRIP) lua.elf
