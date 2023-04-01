/* KallistiOS ##version##

   closedir.c
   Copyright (C)2004 Megan Potter

*/

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <kos/fs.h>

int closedir(DIR * d) {
    if(!d) {
        errno = EBADF;
        return -1;
    }

    fs_close(d->fd);
    free(d);

    return 0;
}
