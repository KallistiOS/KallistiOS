#
# POSIX-style exec() call test
# (c)2002 Megan Potter
#   

TARGET = exec.elf
OBJS = exec.o romdisk.o

SUBTARGET = romdisk/sub.bin
SUBOBJS = sub.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS) $(SUBTARGET) $(SUBOBJS) sub.elf

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS)

$(SUBTARGET): $(SUBOBJS)
	kos-cc -o sub.elf $(SUBOBJS)
	$(KOS_OBJCOPY) -O binary sub.elf $(SUBTARGET)

# You can safely remove the next two targets if you don't use a ROMDISK
romdisk.img: $(SUBTARGET)
	$(KOS_GENROMFS) -f romdisk.img -d romdisk -v -x .keepme

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) sub.elf sub.o romdisk/sub.bin romdisk.img
	$(KOS_STRIP) $(TARGET)

