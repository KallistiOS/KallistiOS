# KallistiOS ##version##
#
# conio/adventure/Makefile
# (c)2002 Dan Potter
#

# Determine on what platform we are running
# TODO: add all detection for others platforms (Linux, OSX and Cygwin...)
ifeq ($(shell uname -s | cut -c-7),MINGW32)
	MINGW32 := 1
endif

EXECUTABLEEXTENSION =
ifdef MINGW32
	EXECUTABLEEXTENSION = .exe
endif

# Program binary
TARGET = adventure.elf

# Script data files
DATA_SOURCE	= glorkz
DATA_TARGET = data.c

# Setup data utility processing
# Under Windows, avoid the "setup.exe" name as it causes problems with the UAC...
SETUP_SOURCE = setup.c
SETUP_TARGET = datagen$(EXECUTABLEEXTENSION)

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

OBJS = porthelper.o crc.o done.o init.o io.o main.o save.o subr.o vocab.o wizard.o data.o

$(DATA_TARGET):
	gcc -o $(SETUP_TARGET) $(SETUP_SOURCE) -DSETUP
	./$(SETUP_TARGET) $(DATA_SOURCE) > $(DATA_TARGET)

clean:
	-rm -f $(TARGET) $(OBJS) $(DATA_TARGET) $(SETUP_TARGET)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS) 
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) $(OBJS) $(DATAOBJS) $(OBJEXTRA) -lconio $(KOS_LIBS)

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	-rm -f $(OBJS) $(DATA_TARGET) $(SETUP_TARGET)
	$(KOS_STRIP) $(TARGET)
