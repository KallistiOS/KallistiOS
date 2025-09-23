# libkosext2fs Makefile
#

TARGET = libkosext2fs.a
OBJS = ext2fs.o bitops.o block.o inode.o superblock.o fs_ext2.o symlink.o \
       directory.o

# Make sure everything compiles nice and cleanly (or not at all).
KOS_CFLAGS += -W -pedantic -Werror -std=gnu99

include $(KOS_BASE)/addons/Makefile.prefab
