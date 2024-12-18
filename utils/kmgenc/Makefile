
# Makefile for the kmgenc program.

CFLAGS = -O2 -Wall -I/usr/local/include
LDFLAGS = -s -lpng -ljpeg -lz -L/usr/local/lib

all: kmgenc

kmgenc: kmgenc.o get_image.o get_image_jpg.o get_image_png.o readpng.o
	$(CC) -o $@ $+ $(LDFLAGS)

clean:
	rm -f kmgenc *.o

