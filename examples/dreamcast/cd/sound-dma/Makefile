#
# CD DMA Example Makefile
# Copyright (C) 2004 Megan Potter
# Copyright (C) 2024 Falco Girgis
# Copyright (C) 2024 Stefanos Kornilios Mitsis Poiitidis
#

TARGET = cd-dma.elf
TARGET_CDI = cd-dma.cdi

OBJS = cd-dma.o

all: $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS)

cdi: cd/test.pcm $(TARGET)
	mkdcdisc -e $(TARGET) -o $(TARGET_CDI) -d cd/ -N

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
