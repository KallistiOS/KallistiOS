#
# Nehe02 example adapted from KGL
# (c)2013-2014 Josh Pearson
# (c)2001 Benoit Miller
#   

TARGET = nehe02.elf
OBJS = nehe02.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) $(OBJEXTRA) -L$(KOS_BASE)/lib -lGL -lm $(KOS_LIBS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)

