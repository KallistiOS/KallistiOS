/* KallistiOS ##version##

   newlib_kill.c
   Copyright (C) 2004 Megan Potter

*/

#include <reent.h>
#include <errno.h>

int _kill_r(struct _reent *reent, int pid, int sig) {
    (void)reent;
    (void)pid;
    (void)sig;

    errno = EINVAL;
    return -1;
}
