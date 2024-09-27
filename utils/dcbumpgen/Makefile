
# Makefile stolen from the kmgenc program.

CFLAGS = -O2 -Wall -I/usr/local/include
LDFLAGS = -s -lpng -ljpeg -lm -lz -L/usr/local/lib

all: dcbumpgen

dcbumpgen: dcbumpgen.o get_image.o get_image_jpg.o get_image_png.o readpng.o
	$(CC) -o $@ $+ $(LDFLAGS)

clean:
	rm -f dcbumpgen *.o

