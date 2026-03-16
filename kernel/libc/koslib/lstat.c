/* KallistiOS ##version##

   lstat.c
   Copyright (C)2026 Yevhen Lohachov
*/

#include <kos/fs.h>

int lstat(const char *restrict path, struct stat *restrict buf) {
    return fs_stat(path, buf, AT_SYMLINK_NOFOLLOW);
}
