#
# KallistiGL test program
# (c)2013-2014 Josh PH3NOM Pearson
# (c)2001 Megan Potter
#   

TARGET = gltest.elf
OBJS = gltest.o pvr-texture.o romdisk.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS)
	kos-cc $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) $(OBJEXTRA) -lGL -lpcx -lkosutils $(KOS_LIBS)

romdisk.img:
	$(KOS_GENROMFS) -f romdisk.img -d romdisk -v

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	rm -f $(OBJS) romdisk.img
	$(KOS_STRIP) $(TARGET)

