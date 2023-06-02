# KallistiOS ##version##
#
# examples/dreamcast/Makefile
# Copyright (C)2003 Megan Potter
#

DIRS = 2ndmix basic libdream kgl hello sound png network vmu conio pvr video \
	   lua parallax modem dreameye sd g1ata lightgun keyboard sdl random
ifdef KOS_CCPLUS
	DIRS += cpp tsunami
endif

all:
	for i in $(DIRS); do $(KOS_MAKE) -C $$i || exit -1; done

clean:
	for i in $(DIRS); do $(KOS_MAKE) -C $$i clean || exit -1; done

dist:
	for i in $(DIRS); do $(KOS_MAKE) -C $$i dist || exit -1; done
