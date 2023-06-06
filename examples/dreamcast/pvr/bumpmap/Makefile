TARGET = bump.elf
OBJS = bump.o romdisk.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS) romdisk/*.kmg romdisk/*.raw

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -lkmg -lkosutils

romdisk.img: romdisk/bricks.kmg romdisk/bumpmap.raw
	$(KOS_GENROMFS) -f romdisk.img -d romdisk -v -x .keepme

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

romdisk/bricks.kmg: bricks.png
	$(KOS_BASE)/utils/vqenc/vqenc -t -v -q -k bricks.png
	mv bricks.kmg romdisk/

romdisk/bumpmap.raw: bumpmap.png
	$(KOS_BASE)/utils/dcbumpgen/dcbumpgen $+ $@

run: $(TARGET)
	$(KOS_LOADER) $(TARGET) -n

dist: $(TARGET)
	-rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)
