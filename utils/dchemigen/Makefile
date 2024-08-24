
# Makefile stolen from the kmgenc program and then the dcbumpgen program.

CFLAGS = -O2 -Wall -DINLINE=inline -I/usr/local/include
LDFLAGS = -s -lpng -ljpeg -lm -lz -L/usr/local/lib

all: dchemigen

dchemigen: dchemigen.o get_image.o get_image_jpg.o get_image_png.o readpng.o
	$(CC) -o $@ $+ $(LDFLAGS)

clean:
	rm -f dchemigen *.o
