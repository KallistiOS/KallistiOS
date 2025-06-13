# KallistiOS Ogg/Vorbis Decoder Library
#
# Library Makefile
# (c)2001 Thorsten Titze
# Based on KOS Makefiles by Megan Potter

TARGET = vorbistest.elf
OBJS = vorbistest.o romdisk.o
KOS_ROMDISK_DIR = romdisk

# You can choose a decoder to test below. Tremor is the integer-only
# version, and oggvorbisplay is the full FP version.
LDLIBS = -ltremor
#LDLIBS = -loggvorbisplay -lvorbis -logg

include $(KOS_BASE)/Makefile.rules
