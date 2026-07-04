/* KallistiOS ##version##

   newlib_fcntl.c
   Copyright (C) 2012 Lawrence Sebald

*/

#include <reent.h>
#include <kos/fs.h>

int _fcntl_r(struct _reent *reent, int fd, int cmd, int arg) {
    (void)reent;
    return fs_fcntl(fd, cmd, arg);
}
