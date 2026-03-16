/* KallistiOS ##version##

   link.c
   Copyright (C)2026 Yevhen Lohachov
*/

#include <kos/fs.h>

int link(const char *oldpath, const char *newpath) {
    return fs_link(oldpath, newpath);
}
