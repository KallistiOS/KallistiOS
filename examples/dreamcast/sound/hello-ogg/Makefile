# KallistiOS Ogg/Vorbis Decoder Library
#
# Library Makefile
# (c)2001 Thorsten Titze
# Based on KOS Makefiles by Megan Potter

TARGET = vorbistest.elf
OBJS = vorbistest.o romdisk.o 

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET) romdisk.*

# You can choose a decoder to test below. Tremor is the integer-only
# version, and oggvorbisplay is the full FP version.
$(TARGET): $(OBJS) 
	kos-cc -o $(TARGET) $(OBJS) -ltremor -lm
#	kos-cc -o $(TARGET) $(OBJS) -loggvorbisplay -lvorbis -logg -lm

romdisk.img:
	$(KOS_GENROMFS) -f romdisk.img -d romdisk -v

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)
#	dc-tool -p -b 115200 -x $(TARGET)

dist: $(TARGET)
	rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)

