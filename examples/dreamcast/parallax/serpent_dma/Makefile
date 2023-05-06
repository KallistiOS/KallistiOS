#
# PVR/Parallax vertex DMA test program
#   

TARGET = serpent.elf
OBJS = serpent.o perfmeter.o romdisk.o

KOS_ROMDISK_DIR = romdisk

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) $(OBJEXTRA) -lparallax -lkosutils -lpng -ljpeg -lkmg -lz -lm $(KOS_LIBS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)

