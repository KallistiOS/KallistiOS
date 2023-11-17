#
# vmu_game test program
# (c)2020 BBHoodsta
#   

# Put the filename of the output binary here
TARGET = vmu_game.elf

# List all of your C files here, but change the extension to ".o"
OBJS = vmu_game.o romdisk.o

KOS_ROMDISK_DIR = romdisk

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)

