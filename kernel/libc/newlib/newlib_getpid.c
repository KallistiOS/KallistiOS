/* KallistiOS ##version##

   newlib_getpid.c
   Copyright (C)2004 Megan Potter

*/

#include <kos/thread.h>
#include <sys/reent.h>

int _getpid_r(struct _reent * re) {
    (void)re;
    return thd_current->tid;
}
