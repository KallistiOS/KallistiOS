# KallistiOS ##version##
#
# examples/dreamcast/sd/mke2fs/Makefile
#

TARGET = mke2fs.elf
OBJS = mke2fs.o

# We need the private headers from libkosext2fs, since we're not using the
# facilities of fs_ext2 here.
CFLAGS := -I$(KOS_BASE)/addons/libkosext2fs -Werror -W -std=gnu99

include $(KOS_BASE)/Makefile.rules
