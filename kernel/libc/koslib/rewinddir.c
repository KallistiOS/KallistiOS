/* KallistiOS ##version##

   rewinddir.c
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2014 Lawrence Sebald

*/

#include <dirent.h>
#include <errno.h>
#include <kos/fs.h>

void rewinddir(DIR *dirp) {
    if(!dirp) {
        errno = EBADF;
        return;
    }

    (void)fs_rewinddir(dirp->fd);
}
