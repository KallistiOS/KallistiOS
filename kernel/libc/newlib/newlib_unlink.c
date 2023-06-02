/* KallistiOS ##version##

   newlib_unlink.c
   Copyright (C)2004 Megan Potter

*/

#include <kos/fs.h>

int _unlink_r(void * reent, const char * fn) {
    (void)reent;
    return fs_unlink(fn);
}
