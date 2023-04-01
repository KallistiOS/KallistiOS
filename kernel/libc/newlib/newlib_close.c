/* KallistiOS ##version##

   newlib_close.c
   Copyright (C)2004 Megan Potter

*/

#include <sys/reent.h>
#include <kos/fs.h>

int _close_r(struct _reent * reent, int f) {
    (void)reent;
    return fs_close(f);
}
