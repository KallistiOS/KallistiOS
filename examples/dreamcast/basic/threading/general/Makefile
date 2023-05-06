# KallistiOS ##version##
#
# basic/threading/Makefile
# (c)2001 Megan Potter
#

TARGET = general_threading_test.elf
OBJS = general_threading_test.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS) 
	kos-cc $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) $(DATAOBJS) $(OBJEXTRA) $(KOS_LIBS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
