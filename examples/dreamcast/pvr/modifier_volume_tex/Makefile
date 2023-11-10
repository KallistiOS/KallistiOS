#
# Basic KallistiOS skeleton / test program
# (c)2001 Megan Potter
#

# Put the filename of the output binary here
TARGET = modifier.elf

# List all of your C files here, but change the extension to ".o"
OBJS = modifier.o romdisk.o

KOS_ROMDISK_DIR = romdisk

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS) romdisk/fruit.kmg

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -lkmg -lpcx -lkosutils

romdisk.img: romdisk/fruit.kmg romdisk/crate.pcx

romdisk/fruit.kmg: fruit.jpg
	$(KOS_BASE)/utils/vqenc/vqenc -t -v -q -k fruit.jpg
	mv fruit.kmg romdisk/

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) romdisk.img romdisk/fruit.kmg
	$(KOS_STRIP) $(TARGET)

