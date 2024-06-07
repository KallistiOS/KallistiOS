#
# Tsunami genmenu test
# Copyright (C)2003 Megan Potter
#   

TARGET = genmenu.elf
OBJS = genmenu.o romdisk.o
KOS_ROMDISK_DIR = romdisk

# GCC versions prior to 11.0 have incomplete C++17 support and need 
# to be forced to use the GNU dialect for std::filesystem support
KOS_CPPFLAGS += -std=gnu++17

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS)
	kos-c++ -o $(TARGET) $(OBJS) -ltsunami -lparallax -lpng -ljpeg -lkmg -lz -lkosutils

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)

