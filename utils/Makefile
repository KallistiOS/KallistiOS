# KallistiOS ##version##
#
# utils/Makefile
# Copyright (C) 2001 Megan Potter
#

SUBDIRS = bin2c bincnv dcbumpgen genromfs kmgenc makeip scramble vqenc wav2adpcm pvrtex

ifeq ($(KOS_SUBARCH), naomi)
	SUBDIRS += naomibintool naominetboot
endif

all: subdirs

clean: clean_subdirs

include $(KOS_BASE)/Makefile.rules
