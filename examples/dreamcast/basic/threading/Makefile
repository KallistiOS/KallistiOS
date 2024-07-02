# KallistiOS ##version##
#
# examples/dreamcast/basic/threading/Makefile
# Copyright (C) 2002 Megan Potter
#

all:
	$(KOS_MAKE) -C compiler_tls
	$(KOS_MAKE) -C general
	$(KOS_MAKE) -C rwsem
	$(KOS_MAKE) -C recursive_lock
	$(KOS_MAKE) -C once
	$(KOS_MAKE) -C tls
	$(KOS_MAKE) -C spinlock_test
	$(KOS_MAKE) -C atomics
	$(KOS_MAKE) -C reentrant_mutex

clean:
	$(KOS_MAKE) -C compiler_tls clean
	$(KOS_MAKE) -C general clean
	$(KOS_MAKE) -C rwsem clean
	$(KOS_MAKE) -C recursive_lock clean
	$(KOS_MAKE) -C once clean
	$(KOS_MAKE) -C tls clean
	$(KOS_MAKE) -C spinlock_test clean
	$(KOS_MAKE) -C atomics clean
	$(KOS_MAKE) -C reentrant_mutex clean

dist:
	$(KOS_MAKE) -C compiler_tls dist
	$(KOS_MAKE) -C general dist
	$(KOS_MAKE) -C rwsem dist
	$(KOS_MAKE) -C recursive_lock dist
	$(KOS_MAKE) -C once dist
	$(KOS_MAKE) -C tls dist
	$(KOS_MAKE) -C spinlock_test dist
	$(KOS_MAKE) -C atomics dist
	$(KOS_MAKE) -C reentrant_mutex dist
