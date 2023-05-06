# KallistiOS ##version##
#
# basic/threading/tls/Makefile
# Copyright (C) 2009 Lawrence Sebald
#

TARGET = tls_test.elf
OBJS = tls_test.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

tls_test.elf: $(OBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) $(DATAOBJS) $(OBJEXTRA) $(KOS_LIBS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)

