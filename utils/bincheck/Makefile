MAKE ?= make

all:
	$(MAKE) -C src
	$(MAKE) -C src install

install: all

clean:
	$(MAKE) -C src clean

distclean:
	$(MAKE) -C src distclean

.PHONY: all install clean distclean
