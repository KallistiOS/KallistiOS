# KallistiOS ##version##
#
# examples/dreamcast/kgl/demos/Makefile
# 
# (c)2014 Josh Pearson
# (c)2002 Paul Boese
# (c)2001-2002 Megan Potter
#
# This will make all the examples, leaving only the elf files.
# If you want to clean everything, use 'clean'.

SUBDIRS = blur mipmap specular multitexture-arrays multitexture-elements

all: $(patsubst %, _dir_%, $(SUBDIRS))

$(patsubst %, _dir_%, $(SUBDIRS)):
	$(MAKE) -C $(patsubst _dir_%, %, $@)

clean: $(patsubst %, _cleandir_%, $(SUBDIRS))

$(patsubst %, _cleandir_%, $(SUBDIRS)):
	$(MAKE) -C $(patsubst _cleandir_%, %, $@) clean

dist: $(patsubst %, _distdir_%, $(SUBDIRS))

$(patsubst %, _distdir_%, $(SUBDIRS)):
	$(MAKE) -C $(patsubst _distdir_%, %, $@) dist


