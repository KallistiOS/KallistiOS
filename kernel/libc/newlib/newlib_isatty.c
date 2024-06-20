/* KallistiOS ##version##

   newlib_isatty.c
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2012 Lawrence Sebald
   Copyright (C) 2024 Andress Barajas

*/

#include <unistd.h>
#include <errno.h>
#include <sys/reent.h>

int isatty(int fd) {
    if(fd < 0) {
        errno = EBADF;
        return 0;
    }

    /* Make sure that stdin, stdout, stderr is shown as a tty, otherwise
       it won't be set as line-buffered. */
    if(fd >= STDIN_FILENO && fd <= STDERR_FILENO)
        return 1;

    return 0;
}

int _isatty_r(struct _reent *reent, int fd) {
    (void)reent;

    return isatty(fd);
}
