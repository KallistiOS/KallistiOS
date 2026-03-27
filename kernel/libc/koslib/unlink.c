/* KallistiOS ##version##

   unlink.c
   Copyright (C)2026 Yevhen Lohachov
*/

#include <kos/fs.h>

int unlink(const char *path) {
    return fs_unlink(path);
}
