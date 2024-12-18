
# Makefile for the vqenc program.

CFLAGS = -O2 -Wall -I/usr/local/include
LDFLAGS = -lpng -ljpeg -lz -lm -L/usr/local/lib

all: vqenc

vqenc: vqenc.o get_image.o get_image_jpg.o get_image_png.o readpng.o
	$(CC) -o $@ $+ $(LDFLAGS)

clean:
	rm -f vqenc *.o

install: all 
	install -m 755 vqenc /usr/bin
