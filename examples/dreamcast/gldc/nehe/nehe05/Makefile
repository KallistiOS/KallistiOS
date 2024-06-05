#
# Nehe05 example adapted from KGL
# (c)2014 Josh Pearson
# (c)2001 Benoit Miller
#   

TARGET = nehe05.elf
OBJS = nehe05.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -L$(KOS_BASE)/lib -lGL

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)

