# KallistiOS ##version##
#
# examples/dreamcast/basic/Makefile
# (c)2002 Megan Potter
#

all:
	$(KOS_MAKE) -C exec
	$(KOS_MAKE) -C threading
	$(KOS_MAKE) -C fpu
	$(KOS_MAKE) -C asserthnd
	$(KOS_MAKE) -C stacktrace
	$(KOS_MAKE) -C mmu
	$(KOS_MAKE) -C stackprotector
	$(KOS_MAKE) -C memtest32

clean:
	$(KOS_MAKE) -C exec clean
	$(KOS_MAKE) -C threading clean
	$(KOS_MAKE) -C fpu clean
	$(KOS_MAKE) -C asserthnd clean
	$(KOS_MAKE) -C stacktrace clean
	$(KOS_MAKE) -C mmu clean
	$(KOS_MAKE) -C stackprotector clean
	$(KOS_MAKE) -C memtest32 clean

dist:
	$(KOS_MAKE) -C exec dist
	$(KOS_MAKE) -C threading dist
	$(KOS_MAKE) -C fpu dist
	$(KOS_MAKE) -C asserthnd dist
	$(KOS_MAKE) -C stacktrace dist
	$(KOS_MAKE) -C mmu dist
	$(KOS_MAKE) -C stackprotector dist
	$(KOS_MAKE) -C memtest32 dist
