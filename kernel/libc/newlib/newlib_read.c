/* KallistiOS ##version##

   newlib_read.c
   Copyright (C)2004 Megan Potter

*/

#include <reent.h>
#include <kos/fs.h>

ssize_t _read_r(struct _reent *reent, int fd, void *buf, size_t cnt) {
    (void)reent;
    return fs_read(fd, buf, cnt);
}
