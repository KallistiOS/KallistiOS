/* KallistiOS ##version##

   readdir.c
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2024 Falco Girgis
*/

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <kos/fs.h>

#ifdef __STRICT_ANSI__
/* Newlib doesn't prototype this function in strict standards compliant mode, so
   we'll do it here. It is still provided either way, but it isn't prototyped if
   we use -std=c17 (or any other non-gnuXX value). */
size_t strnlen(const char *, size_t);
#endif

struct dirent *readdir(DIR *dir) {
    dirent_t *d;
    size_t len;

    if(!dir) {
        errno = EBADF;
        return NULL;
    }

    d = fs_readdir(dir->fd);

    if(!d)
        return NULL;

    dir->d_ent.d_ino = 0;
    dir->d_ent.d_off = 0;
    dir->d_ent.d_reclen = 0;

    if(d->size < 0)
        dir->d_ent.d_type = DT_DIR;
    else
        dir->d_ent.d_type = DT_REG;

    len = strnlen(d->name, sizeof(dir->d_name) - 1);

    strncpy(dir->d_ent.d_name, d->name, len);
    dir->d_ent.d_name[len] = '\0';

    return &dir->d_ent;
}
