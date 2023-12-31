# KallistiOS ##version##
#
# examples/dreamcast/kgl/nehe/Makefile
# (c)2002 Paul Boese
#
# This will make all the examples, leaving only the elf files.
# If you want to clean everything, use 'clean'.

SUBDIRS = nehe02 nehe05 nehe06 nehe08 nehe09 nehe16 nehe26

all: $(patsubst %, _dir_%, $(SUBDIRS))

$(patsubst %, _dir_%, $(SUBDIRS)):
	$(MAKE) -C $(patsubst _dir_%, %, $@)

clean: $(patsubst %, _cleandir_%, $(SUBDIRS))

$(patsubst %, _cleandir_%, $(SUBDIRS)):
	$(MAKE) -C $(patsubst _cleandir_%, %, $@) clean

dist: $(patsubst %, _distdir_%, $(SUBDIRS))

$(patsubst %, _distdir_%, $(SUBDIRS)):
	$(MAKE) -C $(patsubst _distdir_%, %, $@) dist

