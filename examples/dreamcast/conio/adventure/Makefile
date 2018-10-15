# KallistiOS ##version##
#
# conio/adventure/Makefile
# (c)2002 Dan Potter
#

include Makefile.hostdetect

# Program binary
TARGET = adventure.elf

# Script data files
DATA_SOURCE	= glorkz
DATA_TARGET = data.c

# For MinGW/MSYS, MinGW-w64/MSYS2 and Cygwin
ifdef WINDOWS
  EXECUTABLEEXTENSION = .exe
endif

# Setup data utility processing
# Under Windows, avoid the "setup.exe" name as it causes problems with the UAC...
SETUP_SOURCE = setup.c
SETUP_TARGET = datagen$(EXECUTABLEEXTENSION)
SETUP_CFLAGS = -DSETUP

# Only for MinGW-w64/MSYS2, even if x86!
ifdef MINGW64
  SETUP_CFLAGS += -D_MINGW_W64
endif

# Additional configuration
HOST_CC = gcc

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

OBJS = porthelper.o crc.o done.o init.o io.o main.o save.o subr.o vocab.o wizard.o data.o

$(DATA_TARGET):
	$(HOST_CC) -o $(SETUP_TARGET) $(SETUP_SOURCE) $(SETUP_CFLAGS)
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
