#
# OpenGL (c)2013-2014 Josh PH3NOM Pearson
# trimark - GL_TRIANGLES benchmark
# (c)2001 Megan Potter, Paul Boese
#   

TARGET = trimark.elf
OBJS = trimark.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -lGL

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)

