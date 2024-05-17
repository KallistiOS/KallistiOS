TARGET = ghettoplay.elf
OBJS = ghettoplay.o texture.o bkg.o 3dutils.o songmenu.o romdisk.o
KOS_ROMDISK_DIR = romdisk

all: rm-elf $(TARGET)

include ${KOS_BASE}/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET) romdisk.*
	
$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -loggvorbisplay -lvorbis -logg

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)
