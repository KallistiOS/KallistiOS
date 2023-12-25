# libkosfat Makefile
#

TARGET = libkosfat.a
OBJS = fat.o bpb.o fatfs.o directory.o ucs.o fs_fat.o

# Make sure everything compiles nice and cleanly (or not at all).
KOS_CFLAGS += -W -Wextra -pedantic -std=c99

include $(KOS_BASE)/addons/Makefile.prefab
