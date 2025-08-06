/* KallistiOS ##version##

   kos.h
   Copyright (C) 2001 Megan Potter

*/

/** \file   kos.h
    \brief  Include everything KOS has to offer!

    This file includes pretty much every KOS-related header file, so you don't
    have to figure out what you actually need. The ultimate for the truly lazy!

    You may want to include individual header files yourself if you need more
    fine-grained control, as may be more appropriate for some projects.

    \author Megan Potter
*/

#ifndef __KOS_H
#define __KOS_H

/* The ultimate for the truly lazy: include and go! No more figuring out
   which headers to include for your project. */

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <kos/version.h>
#include <kos/fs.h>
#include <kos/fs_romdisk.h>
#include <kos/fs_ramdisk.h>
#include <kos/fs_dev.h>
#include <kos/fs_pty.h>
#include <kos/limits.h>
#include <kos/thread.h>
#include <kos/sem.h>
#include <kos/rwsem.h>
#include <kos/once.h>
#include <kos/tls.h>
#include <kos/mutex.h>
#include <kos/cond.h>
#include <kos/genwait.h>
#include <kos/library.h>
#include <kos/net.h>
#include <kos/nmmgr.h>
#include <kos/exports.h>
#include <kos/dbgio.h>
#include <kos/blockdev.h>
#include <kos/dbglog.h>
#include <kos/elf.h>
#include <kos/fs_socket.h>
#include <kos/string.h>
#include <kos/init.h>
#include <kos/oneshot_timer.h>
#include <kos/regfield.h>

#include <arch/arch.h>
#include <arch/cache.h>
#include <arch/irq.h>
#include <arch/spinlock.h>
#include <arch/timer.h>
#include <arch/types.h>
#include <arch/exec.h>
#include <arch/stack.h>
#include <arch/byteorder.h>
#include <arch/rtc.h>
#include <arch/kos.h>

__END_DECLS

#endif
