# KallistiOS ##version##
#
# examples/dreamcast/gldc/basic/Makefile
#
# (c)2014 Josh Pearson
# (c)2002 Paul Boese
#
# This will make all the examples, leaving only the elf files.
# If you want to clean everything, use 'clean'.

SUBDIRS = elements gl scissor txrenv vq zclip_arrays

all: $(patsubst %, _dir_%, $(SUBDIRS))

$(patsubst %, _dir_%, $(SUBDIRS)):
	$(MAKE) -C $(patsubst _dir_%, %, $@)

clean: $(patsubst %, _cleandir_%, $(SUBDIRS))

$(patsubst %, _cleandir_%, $(SUBDIRS)):
	$(MAKE) -C $(patsubst _cleandir_%, %, $@) clean

dist: $(patsubst %, _distdir_%, $(SUBDIRS))

$(patsubst %, _distdir_%, $(SUBDIRS)):
	$(MAKE) -C $(patsubst _distdir_%, %, $@) dist

