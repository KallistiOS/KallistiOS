/* KallistiOS ##version##

   opendir.c
   Copyright (C)2004 Megan Potter

*/

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <kos/fs.h>

DIR * opendir(const char * name) {
    file_t fd;
    DIR * newd;

    // Try to open the dir itself
    fd = fs_open(name, O_DIR | O_RDONLY);

    if(fd < 0) {
        // VFS will set errno
        return NULL;
    }

    // Ok, got it. Alloc a struct to return.
    newd = malloc(sizeof(DIR));

    if(!newd) {
        errno = ENOMEM;
        return NULL;
    }

    newd->fd = fd;
    memset(&newd->d_ent, 0, sizeof(struct dirent));

    return newd;
}
