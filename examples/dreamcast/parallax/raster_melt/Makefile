TARGET = raster_melt.elf
OBJS = raster_melt.o romdisk.o
KOS_ROMDISK_DIR = romdisk

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -lparallax -ljpeg -lkmg -lpng -lz -lkosutils

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)

