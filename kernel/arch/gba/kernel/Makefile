# KallistiOS 2.0.0
#
# arch/gba/kernel/Makefile
# (c)2000 Dan Potter
#

# Generic kernel abstraction layer: this provides a set of routines
# that the portable part of the kernel expects to find on every
# target processor. Other routines may be present as well, but
# that minimum set must be present.

COPYOBJS = dbgio.o main.o mm.o panic.o
COPYOBJS += syscalls.o timer.o irq.o
COPYOBJS += init_romdisk_default.o init_flags_default.o
COPYOBJS += lazy_porting.o
OBJS = $(COPYOBJS) startup.o
SUBDIRS = 

myall: $(OBJS)
	cp $(COPYOBJS) $(KOS_BASE)/kernel/build/

include $(KOS_BASE)/Makefile.prefab


