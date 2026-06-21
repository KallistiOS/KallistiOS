#
# KallistiOS network/lftpd example
#
# Copyright (C) 2018 Jason von Nieda
#

TARGET = lftpd.elf
OBJS = lftpd.o lftpd_inet.o lftpd_io.o lftpd_log.o lftpd_string.o
KOS_BUILD_SUBARCHS = pristine

KOS_CFLAGS += -Iinclude -I.

all: $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS)

$(TARGET): $(OBJS)
	kos-cc -o $@ $^ -lkosfat

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
