# KallistiOS ##version##
#
# Root Makefile
# Copyright (C) 2003 Megan Potter
# Copyright (C) 2024 Falco Girgis
#

$(if ${KOS_BASE},,$(error KallistiOS environment not found. Did you source environ.sh? Please take a look at doc/README for more info.))

# Make sure things compile nice and cleanly. We don't necessarily want to push
# these flags out on to user code, but it's a good idea to keep them around for
# compiling all of KOS proper.
# I expect everyone to keep their code warning free. Don't make me add -Werror
# here too. ;-)
KOS_CFLAGS += -Wextra -Wno-deprecated

# Add stuff to SUBDIRS to auto-compile it with the big tree.
SUBDIRS = utils kernel addons # examples

all: subdirs

clean: clean_subdirs

distclean: clean
	-rm -f lib/$(KOS_ARCH)/*
	-rm -f addons/lib/$(KOS_ARCH)/*
	-rm -f $(KOS_SYSROOT)

docs:
	doxygen $(KOS_BASE)/doc/Doxyfile

docs_clean:
	-rm -rf $(KOS_BASE)/doc/reference

docs_open: docs
	open $(KOS_BASE)/doc/reference/html/index.html

kos-ports_all:
	$(MAKE) -C $(KOS_PORTS) all

kos-ports_clean:
	$(MAKE) -C $(KOS_PORTS) clean

all_auto_kos_base:
	$(MAKE) all KOS_BASE=$(CURDIR)

clean_auto_kos_base:
	$(MAKE) clean KOS_BASE=$(CURDIR)

include $(KOS_BASE)/Makefile.rules
