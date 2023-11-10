#
# POSIX-style exec() call test
# (c)2002 Megan Potter
#   

TARGET = exec.elf
OBJS = exec.o romdisk.o
KOS_ROMDISK_DIR = romdisk

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

romdisk.img: $(SUBTARGET)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) sub.elf sub.o romdisk/sub.bin romdisk.img
	$(KOS_STRIP) $(TARGET)

