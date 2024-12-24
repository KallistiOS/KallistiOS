#
# Nehe26 example adapted from KGL
# (c)2013-2014 Josh Pearson
# (c)2001 Benoit Miller
#   

TARGET = nehe26.elf
OBJS = nehe26.o romdisk.o
KOS_ROMDISK_DIR = romdisk
RD_DATA = romdisk/sphere.bin romdisk/torus.bin romdisk/tube.bin

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS) $(RD_DATA) data/txt2bin

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -L$(KOS_BASE)/lib -lKGL

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)

data/txt2bin: data/txt2bin.c
	$(CC) -o $@ $<

romdisk.img: $(RD_DATA)

$(RD_DATA): %.bin: data/txt2bin

$(RD_DATA): romdisk/%.bin: data/%.txt
	data/txt2bin $< $@
