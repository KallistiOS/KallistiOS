/* KallistiOS ##version##

   dup.c
   Copyright (C) 2024 Andress Barajas

*/

#include <unistd.h>
#include <kos/fs.h>

int dup(int oldfd) {
    return fs_dup(oldfd);
}
