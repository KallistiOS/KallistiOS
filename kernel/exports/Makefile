# KallistiOS ##version##
#
# kernel/exports/Makefile
# Copyright (C)2003 Megan Potter
#

OBJS = nmmgr.o kernel_exports.o exports.o library.o
SUBDIRS = 

include $(KOS_BASE)/Makefile.prefab

kernel_exports.o: kernel_exports.c

kernel_exports.c: ../exports.txt
	$(KOS_BASE)/utils/genexports/genexports.sh ../exports.txt kernel_exports.c kernel_symtab

clean: defaultclean
	-rm -f kernel_exports.c
