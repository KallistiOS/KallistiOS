/* KallistiOS ##version##

   newlib_fcntl.c
   Copyright (C) 2012 Lawrence Sebald

*/

#include <sys/fcntl.h>

#include <kos/fs.h>

struct _reent;

int _fcntl_r(struct _reent *reent, int fd, int cmd, int arg) {
    (void)reent;
    return fs_fcntl(fd, cmd, arg);
}
