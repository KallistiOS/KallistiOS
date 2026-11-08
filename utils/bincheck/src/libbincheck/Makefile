# libbincheck: scrambled/unscrambled detection for Dreamcast binaries.
# Builds the static library libbincheck.a; the public API is in bincheck.h.
# Link programs using it with -lbincheck -lm.

TARGET = libbincheck.a

OBJECTS = bincheck.o scramble.o

CC = gcc
AR = ar

CFLAGS = -O2 -std=c99 -Wall -Wextra
CPPFLAGS = -I.

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

bincheck.o: bincheck.h scramble.h
scramble.o: scramble.h

clean:
	-rm -f $(TARGET) $(OBJECTS)

distclean: clean

.PHONY: all clean distclean
