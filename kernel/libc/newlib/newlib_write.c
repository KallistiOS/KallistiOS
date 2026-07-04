/* KallistiOS ##version##

   newlib_write.c
   Copyright (C)2004 Megan Potter

*/

#include <reent.h>
#include <kos/fs.h>

ssize_t _write_r(struct _reent *reent, int fd, const void *buf, size_t cnt) {
    (void)reent;
    return fs_write(fd, buf, cnt);
}
