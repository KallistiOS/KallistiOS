#
# Tsunami genmenu test
# Copyright (C)2003 Megan Potter
#   

TARGET = genmenu.elf
OBJS = genmenu.o romdisk.o
KOS_ROMDISK_DIR = romdisk

# GCC versions prior to 11.0 have incomplete C++17 support and need 
# to be forced to use the GNU dialect for std::filesystem support
CXXFLAGS = -std=gnu++17
LDLIBS = -ltsunami -lparallax -lpng -ljpeg -lkmg -lz -lkosutils -lstdc++

include $(KOS_BASE)/Makefile.rules
