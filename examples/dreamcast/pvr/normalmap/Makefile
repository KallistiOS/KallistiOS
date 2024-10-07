# this assumes a recent kallistios version from svn with the
# new compilers, newlib and everything. though with a little
# tweeking, it should probably run fine with older versions.
# 
# Edited for dchemigen compatibility, uncomment lines 27 
# amd 28 for use with your own .raw file, if necessary, 
# comment out Line 20 as well, if you wnat to retain 
# your .raw between builds. -Lobotomy, 2024

LIBS = -lpng -lz -lm

TARGET = normal.elf
OBJS = main.o romdisk.o
KOS_ROMDISK_DIR = romdisk

all:  $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS) 
	rm -f `find -name "*~" -print`
	-rm -f romdisk.img
	-rm -f romdisk/bumpmap.raw

$(TARGET): $(OBJS) 
	kos-c++ -o $(TARGET) $(OBJS) $(LIBS)

romdisk.img: romdisk/bumpmap.raw

romdisk/bumpmap.raw: romdisk/bumpmap.png
	$(KOS_BASE)/utils/dchemigen/dchemigen $+ $@

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	rm -f $(OBJS)
	$(KOS_STRIP)  $(TARGET)
	sh-elf-objcopy -O binary $(TARGET) $(TARGET).bin
	scramble $(TARGET).bin 1ST_READ.BIN

