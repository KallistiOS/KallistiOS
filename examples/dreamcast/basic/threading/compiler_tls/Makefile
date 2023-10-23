# KallistiOS ##version##
#
# basic/threading/compiler_tls/Makefile
#
# Copyright (C) 2001 Megan Potter
# Copyright (C) 2023 Colton Pawielski
#

TARGET = compiler_tls.elf
OBJS = compiler_tls.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS) 
	kos-cc -o $(TARGET) -ftls-model=local-exec $(OBJS) -Wl,-Map=compiler_tls.map

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
