/* KallistiOS ##version##

   newlib_fork.c
   Copyright (C)2004 Megan Potter

*/

#include <reent.h>
#include <errno.h>

int _fork_r(struct _reent *reent) {
    reent->_errno = EAGAIN;
    return -1;
}
