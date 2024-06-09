#
# DC Clock App
# Based on Peter Hatch's Font Test
# (c)2002 Megan Potter
#   

TARGET = clock.elf
OBJS = clock.o romdisk.o
KOS_ROMDISK_DIR = romdisk

KOS_CPPFLAGS += -std=gnu++17

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS) 

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS)
	kos-c++ -o $(TARGET) $(OBJS) -ldcplib

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)

