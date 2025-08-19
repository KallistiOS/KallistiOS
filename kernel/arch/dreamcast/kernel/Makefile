# KallistiOS ##version##
#
# arch/dreamcast/kernel/Makefile
# (c)2000 Megan Potter
#

# Generic kernel abstraction layer: this provides a set of routines
# that the portable part of the kernel expects to find on every
# target processor. Other routines may be present as well, but
# that minimum set must be present.

COPYOBJS = cache.o entry.o irq.o init.o mm.o panic.o
COPYOBJS += rtc.o timer.o perfctr.o perf_monitor.o
COPYOBJS += init_flags_default.o
COPYOBJS += mmu.o itlb.o
COPYOBJS += exec.o execasm.o stack.o gdb_stub.o thdswitch.o tls_static.o arch_exports.o
OBJS = $(COPYOBJS) startup.o
SUBDIRS =

myall: $(OBJS)
	-cp $(COPYOBJS) $(KOS_BASE)/kernel/build/
	-cp startup.o $(KOS_BASE)/lib/$(KOS_ARCH)/_kos_startup.o

include $(KOS_BASE)/Makefile.prefab

arch_exports.o: arch_exports.c

arch_exports.c: ../exports-$(KOS_SUBARCH).txt
	$(KOS_BASE)/utils/genexports/genexports.sh ../exports-$(KOS_SUBARCH).txt arch_exports.c arch_symtab

clean:
	-rm -f $(OBJS)
	-rm -f arch_exports.c
	-rm -f arch_exports.c
