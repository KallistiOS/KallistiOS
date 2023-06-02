/* KallistiOS ##version##

   newlib_lseek.c
   Copyright (C)2004 Megan Potter

*/

#include <kos/fs.h>

off_t _lseek_r(void * reent, int fd, off_t pos, int whence) {
    (void)reent;
    return fs_seek(fd, pos, whence);
}
