#
# Basic KallistiOS skeleton / test program
# (c)2001 Megan Potter
#

# Put the filename of the output binary here
TARGET = modifier.elf

# List all of your C files here, but change the extension to ".o"
OBJS = modifier.o romdisk.o

LDLIBS = -lkmg -lpcx -lkosutils

KOS_ROMDISK_DIR = romdisk

include $(KOS_BASE)/Makefile.rules

clean_extra:
	-rm -f romdisk/fruit.kmg

clean: clean_extra

romdisk.img: romdisk/fruit.kmg

romdisk/fruit.kmg: fruit.jpg
	$(KOS_BASE)/utils/vqenc/vqenc -t -v -q -k fruit.jpg
	mv fruit.kmg romdisk/
