# KallistiOS 1.1.6
#
# pogo-keen/Makefile
# (c)2002 Gil Megidish
#

TARGET = pogo-keen.elf
OBJS = pogo.o romdisk.o

all: pogo-keen.gba

KOS_ROMDISK_DIR = romdisk_boot
include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(OBJS)
	-rm -f pogo-keen.gba
	-rm -f pogo-keen.elf
	-rm -f romdisk.*	

pogo-keen.gba: $(TARGET)
	$(KOS_BASE)/utils/gba-elf2bin/gba-elf2bin $(TARGET) pogo-keen.gba
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -lkosutils

#romdisk_boot.o:
#	$(KOS_GENROMFS) -f romdisk_boot.img -d romdisk_boot -v
#	$(KOS_BASE)/utils/bin2o/bin2o romdisk_boot.img romdisk_boot romdisk_boot.o

