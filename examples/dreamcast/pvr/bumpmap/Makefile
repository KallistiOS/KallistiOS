TARGET = bump.elf
OBJS = bump.o romdisk.o
KOS_ROMDISK_DIR = romdisk
LDLIBS = -lkmg -lkosutils

include $(KOS_BASE)/Makefile.rules

clean_extra:
	-rm -f romdisk/bricks.kmg romdisk/bumpmap.raw

clean: clean_extra

romdisk.img: romdisk/bricks.kmg romdisk/bumpmap.raw

romdisk/bricks.kmg: bricks.png
	$(KOS_BASE)/utils/vqenc/vqenc -t -v -q -k bricks.png
	mv bricks.kmg romdisk/

romdisk/bumpmap.raw: bumpmap.png
	$(KOS_BASE)/utils/dcbumpgen/dcbumpgen $+ $@
