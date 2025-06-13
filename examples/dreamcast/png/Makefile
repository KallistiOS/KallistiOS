TARGET = example.elf
OBJS = example.o wfont.o romdisk.o 
KOS_ROMDISK_DIR = romdisk
LDLIBS = -lpng -lz

include $(KOS_BASE)/Makefile.rules

wfont.o: wfont.bin
	$(KOS_BASE)/utils/bin2o/bin2o wfont.bin wfont wfont.o
