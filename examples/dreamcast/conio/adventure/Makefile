# KallistiOS ##version##
#
# conio/adventure/Makefile
# (c)2002 Megan Potter
#

# Program binary
TARGET = adventure.elf
OBJS = porthelper.o crc.o done.o init.o io.o adventure.o save.o subr.o vocab.o wizard.o data.o globalvars.o

# Script data files
DATA_SOURCE = glorkz
DATA_TARGET = data.c

# For MinGW/MSYS, MinGW-w64/MSYS2 and Cygwin
ifeq ($(shell echo $(OS)),Windows_NT)
  EXECUTABLEEXTENSION := .exe
endif

# Setup data utility processing
# Under Windows, avoid the "setup.exe" name as it causes problems with the UAC...
SETUP_SOURCE = setup.c
SETUP_TARGET = datagen$(EXECUTABLEEXTENSION)
SETUP_CFLAGS = -DSETUP

# Additional configuration
HOST_CC = gcc

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

$(DATA_TARGET):
	$(HOST_CC) -o $(SETUP_TARGET) $(SETUP_SOURCE) $(SETUP_CFLAGS)
	./$(SETUP_TARGET) $(DATA_SOURCE) > $(DATA_TARGET)

clean: rm-elf
	-rm -f $(OBJS) $(DATA_TARGET) $(SETUP_TARGET)

rm-elf:
	-rm -f $(TARGET)

$(TARGET): $(OBJS) 
	kos-cc -o $(TARGET) $(OBJS) -lconio

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	-rm -f $(OBJS) $(DATA_TARGET) $(SETUP_TARGET)
	$(KOS_STRIP) $(TARGET)
