# KallistiOS ##version##
#
# examples/dreamcast/g1ata/atatest/Makefile
#

TARGET = atatest.elf
OBJS = atatest.o
KOS_BUILD_SUBARCHS = pristine

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS) 

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
