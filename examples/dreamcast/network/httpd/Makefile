#
# KallistiOS network/httpd example
# Copyright (C) 2002 Megan Potter
#

# Put the filename of the output binary here
TARGET = httpd.elf

# List all of your C files here, but change the extension to ".o"
OBJS = simhost.o httpd.o romdisk.o

# Only build for pristine subarch (aka. "dreamcast")
KOS_BUILD_SUBARCHS = pristine

KOS_ROMDISK_DIR = romdisk

include $(KOS_BASE)/Makefile.rules
