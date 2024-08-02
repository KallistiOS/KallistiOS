# KallistiOS ##version##
#
# utils/Makefile
# Copyright (C) 2001 Megan Potter
#

SUBDIRS = bin2c bincnv dcbumpgen genromfs kmgenc makeip scramble vqenc wav2adpcm

ifeq ($(KOS_SUBARCH), naomi)
	DIRS += naomibintool naominetboot
endif

all: subdirs

clean: clean_subdirs

include $(KOS_BASE)/Makefile.rules
