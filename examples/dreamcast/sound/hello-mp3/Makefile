
TARGET = mp3test.elf
OBJS = mp3test.o romdisk.o 

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS) 
	kos-cc -o $(TARGET) $(OBJS)  $(DATAOBJS) $(OBJEXTRA) -lmp3 -lm

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

