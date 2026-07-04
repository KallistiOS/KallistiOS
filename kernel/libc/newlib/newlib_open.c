/* KallistiOS ##version##

   newlib_open.c
   Copyright (C)2004 Megan Potter

*/

#include <reent.h>
#include <kos/fs.h>

int _open_r(struct _reent *reent, const char *f, int flags, int mode) {
    (void)reent;
    (void)mode;
    return fs_open(f, flags);
}
