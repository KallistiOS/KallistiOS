#
# Tsunami font test
# (c)2002 Megan Potter
#   

TARGET = font.elf
OBJS = font.o romdisk.o
KOS_ROMDISK_DIR = romdisk

# GCC versions prior to 11.0 have incomplete C++17 support and need 
# to be forced to use the GNU dialect for std::filesystem support
CXXFLAGS = -std=gnu++17

LDLIBS = -ltsunami -lstdc++ -lparallax -lpng -ljpeg -lkmg -lz -lkosutils

include $(KOS_BASE)/Makefile.rules
