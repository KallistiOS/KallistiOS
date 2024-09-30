# KallistiOS ##version##
#
# examples/dreamcast/sd/mke2fs/Makefile
#

TARGET = mke2fs.elf
OBJS = mke2fs.o

# We need the private headers from libkosext2fs, since we're not using the
# facilities of fs_ext2 here.
KOS_CFLAGS := -I$(KOS_BASE)/addons/libkosext2fs -Werror -W -std=gnu99 $(KOS_CFLAGS)

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
