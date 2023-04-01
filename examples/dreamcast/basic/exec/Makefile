#
# POSIX-style exec() call test
# (c)2002 Megan Potter
#   

TARGET = exec.elf
OBJS = exec.o

SUBTARGET = romdisk/sub.bin
SUBOBJS = sub.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS) $(SUBTARGET) $(SUBOBJS) sub.elf romdisk.*

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS) romdisk.o
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) romdisk.o $(OBJEXTRA) $(KOS_LIBS)

$(SUBTARGET): $(SUBOBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o sub.elf $(KOS_START) \
		$(SUBOBJS) $(OBJEXTRA) $(KOS_LIBS)
	$(KOS_OBJCOPY) -O binary -R .stack sub.elf $(SUBTARGET)

# You can safely remove the next two targets if you don't use a ROMDISK
romdisk.img: $(SUBTARGET)
	$(KOS_GENROMFS) -f romdisk.img -d romdisk -v

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	rm -f $(OBJS) sub.elf sub.o romdisk/sub.bin romdisk.o romdisk.img
	$(KOS_STRIP) $(TARGET)

