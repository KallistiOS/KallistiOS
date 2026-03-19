/* KallistiOS ##version##

   stat.c
   Copyright (C)2026 Yevhen Lohachov
*/

#include <kos/fs.h>

int stat(const char *restrict path, struct stat *restrict buf) {
    return fs_stat(path, buf, 0);
}
