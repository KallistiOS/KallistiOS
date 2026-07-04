/* KallistiOS ##version##

   newlib_execve.c
   Copyright (C)2004 Megan Potter

*/

#include <reent.h>
#include <errno.h>

int _execve_r(struct _reent *reent, const char *name, char *const argv[], char *const env[]) {
    (void)name;
    (void)argv;
    (void)env;

    reent->_errno = EAGAIN;
    return -1;
}
