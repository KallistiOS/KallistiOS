#
# Basic KallistiOS skeleton / test program
# (c)2001 Megan Potter
#

# Put the filename of the output binary here
TARGET = modifier.elf

# List all of your C files here, but change the extension to ".o"
OBJS = modifier.o romdisk.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS) romdisk.* romdisk/fruit.kmg

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) $(OBJEXTRA) -lkmg -lpcx -lkosutils $(KOS_LIBS)

romdisk.img: romdisk/fruit.kmg romdisk/crate.pcx
	$(KOS_GENROMFS) -f romdisk.img -d romdisk -v

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

romdisk/fruit.kmg: fruit.jpg
	$(KOS_BASE)/utils/vqenc/vqenc -t -v -q -k fruit.jpg
	mv fruit.kmg romdisk/

run: $(TARGET)
	$(KOS_LOADER) $(TARGET) -n

dist:
	rm -f $(OBJS) romdisk.* romdisk/fruit.kmg
	$(KOS_STRIP) $(TARGET)

