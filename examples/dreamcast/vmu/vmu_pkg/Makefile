#
# vmu_pkg test program
# (c)2002 Megan Potter
#   

# Put the filename of the output binary here
TARGET = vmu.elf

# List all of your C files here, but change the extension to ".o"
OBJS = vmu.o romdisk.o

KOS_ROMDISK_DIR = romdisk

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS) romdisk.img

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)

