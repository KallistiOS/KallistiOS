/* KallistiOS ##version##

   newlib_getpid.c
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2024 Falco Girgis

*/

#include <kos/thread.h>
#include <sys/reent.h>

int _getpid_r(struct _reent *re) {
    (void)re;
    return KOS_PID;
}
