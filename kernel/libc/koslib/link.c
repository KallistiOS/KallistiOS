/* KallistiOS ##version##

   pathconf.c
   Copyright (C)2026
*/

#include <kos/fs.h>

int link(const char *oldpath, const char *newpath) {
    return fs_link(oldpath, newpath);
}
