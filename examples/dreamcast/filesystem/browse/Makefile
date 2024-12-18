#
# fs_* test program
#
# Copyright (C) 2024 Andress Barajas
#   

TARGET = browse.elf
OBJS = browse.o romdisk.o
KOS_ROMDISK_DIR = romdisk

all: clean $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS) 
	kos-cc -o $(TARGET) $(OBJS) -lkosfat

stages:
	$(KOS_GENROMFS) -f stage1.img -d stage1 -v
	$(KOS_GENROMFS) -f stage2.img -d stage1 -v

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)
