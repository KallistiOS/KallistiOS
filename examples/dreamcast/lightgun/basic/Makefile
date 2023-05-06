# KallistiOS ##version##
#
# lightgun/basic/Makefile
#

TARGET = lightgun.elf
OBJS = lightgun.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) $(DATAOBJS) $(OBJEXTRA)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
