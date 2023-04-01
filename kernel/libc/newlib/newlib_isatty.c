/* KallistiOS ##version##

   newlib_isatty.c
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2012 Lawrence Sebald

*/

#include <sys/reent.h>

int isatty(int fd) {
    /* Make sure that stdin, stdout, and stderr are shown as ttys, otherwise
       they won't be set as line-buffered. */
    if(fd >= 0 && fd <= 2) {
        return 1;
    }

    return 0;
}

int _isatty_r(struct _reent *reent, int fd) {
    (void)reent;

    /* Make sure that stdin, stdout, and stderr are shown as ttys, otherwise
       they won't be set as line-buffered.*/
    if(fd >= 0 && fd <= 2) {
        return 1;
    }

    return 0;
}
