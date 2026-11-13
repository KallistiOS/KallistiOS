TARGET = bincheck

VERSION = 1.0.0

LIBDIR = libbincheck
LIB = $(LIBDIR)/libbincheck.a
OBJECTS = main.o

CC = gcc
STRIP = strip

CFLAGS = -O2 -std=c99 -Wall -Wextra
CPPFLAGS = -I$(LIBDIR) -DBINCHECK_VERSION=\"$(VERSION)\"
LDLIBS = -lm

INSTALLDIR = ..

EXECUTABLEEXTENSION =
ifeq ($(shell echo $(OS)),Windows_NT)
  EXECUTABLEEXTENSION := .exe
endif

OUTPUT = $(TARGET)$(EXECUTABLEEXTENSION)

all: $(OUTPUT)

$(OUTPUT): $(OBJECTS) $(LIB)
	$(CC) $(LDFLAGS) -o $(OUTPUT) $(OBJECTS) $(LIB) $(LDLIBS)
	$(STRIP) $(OUTPUT)

# Always delegate to the library Makefile which knows its own dependencies
$(LIB): FORCE
	$(MAKE) -C $(LIBDIR)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

main.o: $(LIBDIR)/bincheck.h Makefile

install: $(OUTPUT)
	mkdir -p $(INSTALLDIR)
	mv $(OUTPUT) $(INSTALLDIR)

clean:
	-rm -f $(OUTPUT) *.o
	$(MAKE) -C $(LIBDIR) clean

distclean: clean
	-rm -f $(INSTALLDIR)/$(OUTPUT)

FORCE:

.PHONY: all install clean distclean FORCE
