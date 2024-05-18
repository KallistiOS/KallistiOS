#
# OpenGL (c)2013-2014 Josh PH3NOM Pearson
# tristripmark - triangle strip benchmark program 
# (c)2001 Megan Potter, Paul Boese
#   

# Put the filename of the output binary here
TARGET = tristripmark.elf
OBJS = tristripmark.o

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

