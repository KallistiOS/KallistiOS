#
# Raylib 2D Image Example
# Author: Neila Hawkins
#

CC=kos-cc

TARGET=rayimages.elf
SRC=main.c

LIBS=-lraylib -lGL
CFLAGS=-std=c11 -g

KOS_ROMDISK_DIR = images

KOS_BUILD_SUBARCHS = pristine

all: $(TARGET)

include $(KOS_BASE)/Makefile.rules

$(TARGET): $(SRC) Makefile romdisk.o
	$(CC) -o $(TARGET) $(SRC) romdisk.o $(CFLAGS) $(LIBS)

clean:
	rm $(TARGET) romdisk.img romdisk.o
