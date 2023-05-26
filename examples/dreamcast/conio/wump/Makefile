# KallistiOS ##version##
#
# conio/wump/Makefile
# (c)2002 Megan Potter
#

TARGET = wump.elf
OBJS = wump.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	rm -f $(OBJS)

rm-elf:
	rm -f $(TARGET)

$(TARGET): $(OBJS) 
	kos-cc -o $(TARGET) $(OBJS) -lconio

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
