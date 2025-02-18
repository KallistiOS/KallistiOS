/* KallistiOS ##version##

   newlib_isatty.c
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2012 Lawrence Sebald
   Copyright (C) 2024 Andress Barajas

*/

#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/reent.h>
#include <kos/fs.h>

int isatty(int fd) {
    vfs_handler_t *vh;

    if(fd < 0) {
        errno = EBADF;
        return 0;
    }

    /* Make sure that stdin is shown as a tty, otherwise
       it won't be set as line-buffered. */
    if(fd == STDIN_FILENO)
        return 1;

    vh = fs_get_handler(fd);

    if(vh == NULL)
        return 0;

    /* pty is the only tty we support */
    if(!strcmp(vh->nmmgr.pathname,"/pty"))
        return 1;
    else {
        errno = ENOTTY;
        return 0;
    }
}

int _isatty_r(struct _reent *reent, int fd) {
    (void)reent;

    return isatty(fd);
}
