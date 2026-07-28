# KallistiOS ##version##
#
# arch/xbox/kernel Makefile
#

COPYOBJS = gdt.o gdtload.o init.o irq.o irqentry.o pic.o stack.o thdswitch.o timer.o tls_static.o
OBJS = $(COPYOBJS) startup.o
SUBDIRS =

myall: $(OBJS)
	-cp $(COPYOBJS) $(KOS_BASE)/kernel/build/
	@mkdir -p $(KOS_BASE)/lib/$(KOS_ARCH)
	cp startup.o $(KOS_BASE)/lib/$(KOS_ARCH)/_kos_startup.o

include $(KOS_BASE)/Makefile.prefab
