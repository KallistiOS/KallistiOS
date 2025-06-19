# KallistiOS ##version##
#
# arch/dreamcast/kernel/Makefile
# (c)2000 Megan Potter
#

# Generic kernel abstraction layer: this provides a set of routines
# that the portable part of the kernel expects to find on every
# target processor. Other routines may be present as well, but
# that minimum set must be present.

COPYOBJS = banner.o cache.o entry.o irq.o init.o mm.o panic.o
COPYOBJS += rtc.o timer.o wdt.o perfctr.o perf_monitor.o
COPYOBJS += init_flags_default.o
COPYOBJS += mmu.o itlb.o
COPYOBJS += exec.o execasm.o stack.o gdb_stub.o thdswitch.o tls_static.o arch_exports.o
COPYOBJS += uname.o
OBJS = $(COPYOBJS) startup.o
SUBDIRS =

CRT1_PATH = $(shell $(KOS_CC) $(KOS_CFLAGS) -print-file-name=crt1.o)

myall: $(OBJS)
	-cp $(COPYOBJS) $(KOS_BASE)/kernel/build/
	-rm banner.h authors.h banner.o uname.o
	-cp startup.o $(CRT1_PATH)

include $(KOS_BASE)/Makefile.prefab

uname.o: uname.c

uname.c: banner.h

banner.o: banner.c

banner.c: banner.h authors.h

banner.h: make_banner.sh
	./make_banner.sh

authors.h: make_authors.awk
	awk -f make_authors.awk < ../../../../AUTHORS > authors.h

arch_exports.o: arch_exports.c

arch_exports.c: ../exports-$(KOS_SUBARCH).txt
	$(KOS_BASE)/utils/genexports/genexports.sh ../exports-$(KOS_SUBARCH).txt arch_exports.c arch_symtab

clean:
	-rm -f banner.h authors.h
	-rm -f $(OBJS)
	-rm -f arch_exports.c
	-rm -f arch_exports.c

.PHONY: banner.h authors.h
