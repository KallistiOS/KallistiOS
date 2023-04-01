# KallistiOS Ogg/Vorbis Decoder Library
#
# Library Makefile
# (c)2001 Thorsten Titze
# Based on KOS Makefiles by Megan Potter

TARGET = vorbistest.elf
OBJS = romdisk.o vorbistest.o

all: clean $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS)
	-rm -f romdisk.o romdisk.img

# You can choose a decoder to test below. Tremor is the integer-only
# version, and oggvorbisplay is the full FP version.
$(TARGET): $(OBJS) 
	kos-cc -o $(TARGET) $(OBJS) $(DATAOBJS) $(OBJEXTRA) -ltremor -lm
#	kos-cc -o $(TARGET) $(OBJS) $(DATAOBJS) $(OBJEXTRA) -loggvorbisplay -lvorbis -logg -lm

romdisk.img:
	$(KOS_GENROMFS) -f romdisk.img -d romdisk -v

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

run: 
	$(KOS_LOADER) $(TARGET)
#	dc-tool -p -b 115200 -x $(TARGET)

dist:
	rm -f $(OBJS) romdisk.o romdisk.img
	$(KOS_STRIP) $(TARGET)

