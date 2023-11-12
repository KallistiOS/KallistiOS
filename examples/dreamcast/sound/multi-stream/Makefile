# KallistiOS ##version##
#
# examples/dreamcast/sound/multi-stream/Makefile
#

TARGET = multistream.elf
OBJS = romdisk.o main.o
KOS_ROMDISK_DIR = romdisk

all: clean $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS)
	-rm -f romdisk.o romdisk.img

$(TARGET): $(OBJS) 
	kos-cc -o $(TARGET) $(OBJS) $(DATAOBJS) $(OBJEXTRA) -lwav

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	rm -f $(OBJS) romdisk.o romdisk.img
	$(KOS_STRIP) $(TARGET)

