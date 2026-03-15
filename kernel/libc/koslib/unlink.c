/* KallistiOS ##version##

   pathconf.c
   Copyright (C)2026
*/

#include <kos/fs.h>

int unlink(const char *path) {
    return fs_unlink(path);
}
