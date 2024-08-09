# KallistiOS ##version##
#
# examples/dreamcast/sd/ext2fs/Makefile
#

TARGET = sd-ext2fs.elf
OBJS = sd-ext2fs.o

# Uncomment the next line to enable write support in the example.
#KOS_CFLAGS += -DENABLE_WRITE

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -lkosext2fs

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
