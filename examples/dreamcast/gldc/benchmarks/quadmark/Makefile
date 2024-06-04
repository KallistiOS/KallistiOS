#
# OpenGL (c)2013-2014 Josh PH3NOM Pearson
# quadmark - GL_QUAD benchmark program 
# (c)2001 Megan Potter, Paul Boese
#   

# Put the filename of the output binary here
TARGET = quadmark.elf
OBJS = quadmark.o

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

