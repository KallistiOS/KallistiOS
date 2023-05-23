#
# VMUBeep Test 2
# (c)2008 Donald Haase
#
# Interface based on: 
#
# Rumble2 Test
# (c)2004 Derek Bradley
#   

NAME = beep
TARGET = $(NAME).elf
OBJS = beep.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS) $(NAME).bin romdisk.*

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS) romdisk.o
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) romdisk.o $(OBJEXTRA) -lparallax -lpng -ljpeg -lkmg -lz -lkosutils -lm $(KOS_LIBS)

romdisk.img:
	$(KOS_GENROMFS) -f romdisk.img -d romdisk -v

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	rm -f $(OBJS) romdisk.o romdisk.img
	$(KOS_OBJCOPY) -O binary -R .stack $(TARGET) $(NAME).bin	

