# KallistiOS ##version##
#
# utils/Makefile
# Copyright (C) 2001 Megan Potter
#

DIRS = bin2c bincnv dcbumpgen genromfs kmgenc makeip scramble vqenc wav2adpcm pvrtex

ifeq ($(KOS_SUBARCH), naomi)
	DIRS += naomibintool naominetboot
endif

# Ok for these to fail atm...

all:
	for i in $(DIRS); do $(KOS_MAKE) -C $$i; done

clean:
	for i in $(DIRS); do $(KOS_MAKE) -C $$i clean; done
