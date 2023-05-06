# KallistiOS ##version##
#
# conio/wump/Makefile
# (c)2002 Megan Potter
#

TARGET = wump.elf
OBJS = wump.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS)

rm-elf:
	-rm -f $(TARGET)

wump.elf: $(OBJS) 
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) $(DATAOBJS) $(OBJEXTRA) -lconio $(KOS_LIBS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
