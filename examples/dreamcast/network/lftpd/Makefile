#
# KallistiOS network/lftpd example
# Copyright (C) 2026 BruceLeet
# Copyright (C) 2026 Paul Cercueil
# Copyright (C) 2026 Andy Barajas
#

TARGET = lftpd.elf
OBJS = lftpd.o
KOS_BUILD_SUBARCHS = pristine

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -llftpd -lkosfat

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
