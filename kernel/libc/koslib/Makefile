# KallistiOS ##version##
#
# kernel/libc/koslib/Makefile
# Copyright (C) 2004 Megan Potter
#

# In here we have pieces of the KOS libc that are either too custom or too
# useful in the context of KOS to go with the Newlib defaults.

OBJS = abort.o byteorder.o memset2.o memset4.o memcpy2.o memcpy4.o \
	assert.o dbglog.o malloc.o \
	opendir.o readdir.o closedir.o rewinddir.o scandir.o seekdir.o \
	telldir.o usleep.o inet_addr.o realpath.o getcwd.o chdir.o mkdir.o \
	creat.o sleep.o rmdir.o rename.o inet_pton.o inet_ntop.o \
	inet_ntoa.o inet_aton.o poll.o select.o symlink.o readlink.o \
	gethostbyname.o getaddrinfo.o dirfd.o nanosleep.o basename.o dirname.o \
	sched_yield.o dup.o dup2.o pipe.o gmon.o

include $(KOS_BASE)/Makefile.prefab
