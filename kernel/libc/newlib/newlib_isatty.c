/* KallistiOS ##version##

   newlib_isatty.c
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2012 Lawrence Sebald
   Copyright (C) 2024 Andress Barajas

*/

#include <unistd.h>
#include <errno.h>

#include <sys/reent.h>
#include <sys/termios.h>

int isatty(int fd) {
    struct termios term;

    if(fd < 0) {
        errno = EBADF;
        return 0;
    }

    /* Make sure that stdin is shown as a tty, otherwise
       it won't be set as line-buffered. */
    if(fd == STDIN_FILENO)
        return 1;

    return tcgetattr(fd, &term) == 0;
}

int _isatty_r(struct _reent *reent, int fd) {
    (void)reent;

    return isatty(fd);
}
