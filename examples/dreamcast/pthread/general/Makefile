# KallistiOS ##version##
#
# pthread/general/Makefile
# Copyright (C) 2023 Lawrence Sebald
#

TARGET = general_pthread_test.elf
OBJS = general_pthread_test.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -lpthread

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
