# KallistiOS ##version##
#
# basic/bfont/Makefile
# (c)2002 Megan Potter
#

TARGET = bfont.elf
OBJS = bfont.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS) 
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) $(OBJS) $(DATAOBJS) $(OBJEXTRA) $(KOS_LIBS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
