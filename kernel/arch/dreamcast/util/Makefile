# KallistiOS ##version##
#
# arch/dreamcast/util/Makefile
# Copyright (C) 2001 Megan Potter
#

OBJS = vmu_fb.o vmu_pkg.o vmu_printf.o screenshot.o minifont.o
SUBDIRS =

ifneq ($(KOS_SUBARCH), naomi)
	OBJS += fb_console.o
else
	OBJS += fb_console_naomi.o
endif

include $(KOS_BASE)/Makefile.prefab
