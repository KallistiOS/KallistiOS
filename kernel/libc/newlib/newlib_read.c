/* KallistiOS ##version##

   newlib_read.c
   Copyright (C)2004 Megan Potter

*/

#include <kos/fs.h>

long _read_r(void * reent, int fd, void * buf, size_t cnt) {
    (void)reent;
    return fs_read(fd, buf, cnt);
}
