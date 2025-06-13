#
# POSIX-style exec() call test
# (c)2002 Megan Potter
#   

TARGET = exec.elf
OBJS = exec.o romdisk.o
KOS_ROMDISK_DIR = romdisk

SUBTARGET = romdisk/sub.bin
SUBOBJS = sub.o

include $(KOS_BASE)/Makefile.rules

clean_sub:
	-rm -f $(SUBTARGET) $(SUBOBJS) sub.elf

$(SUBTARGET): $(SUBOBJS)
	kos-cc -o sub.elf $(SUBOBJS)
	$(KOS_OBJCOPY) -O binary sub.elf $(SUBTARGET)

romdisk.img: $(SUBTARGET)

clean: clean_sub
