#
# KallistiGL Direct Rendering test program
# (c)2002 Megan Potter
#   

TARGET = bubbles.elf
OBJS = bubbles.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) $(OBJEXTRA) -lparallax -lGL $(KOS_LIBS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)

