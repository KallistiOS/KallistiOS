
TARGET = example.elf
OBJS = example.o wfont.o romdisk_boot.o 

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

KOS_LOCAL_CFLAGS = -I$(KOS_BASE)/addons/zlib
	
clean: rm-elf
	-rm -f $(OBJS)
	
rm-elf:
	-rm -f $(TARGET) romdisk_boot.*

$(TARGET): $(OBJS) 
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $@ $(KOS_START) $^ -lpng -lz -lm $(KOS_LIBS)

wfont.o: wfont.bin
	$(KOS_BASE)/utils/bin2o/bin2o $< wfont $@

romdisk_boot.img:
	$(KOS_GENROMFS) -f $@ -d romdisk_boot -v

romdisk_boot.o: romdisk_boot.img
	$(KOS_BASE)/utils/bin2o/bin2o $< romdisk_boot $@

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	-rm -f $(OBJS) romdisk_boot.img
	$(KOS_STRIP) example.elf


