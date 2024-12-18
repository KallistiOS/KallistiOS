#
# Basic KallistiOS skeleton / test program
# (c)2001 Megan Potter
#   

# Put the filename of the output binary here
TARGET = ping.elf

# List all of your C files here, but change the extension to ".o"
OBJS = ping.o

# Only build for pristine subarch (aka. "dreamcast")
KOS_BUILD_SUBARCHS = pristine

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET) -n

dist: $(TARGET)
	-rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)

