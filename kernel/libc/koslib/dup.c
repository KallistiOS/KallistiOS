/* KallistiOS ##version##

   dup.c
   Copyright (C) 2024 Andress Barajas

*/

#include <unistd.h>
#include <kos/fs.h>

int dup(int oldfd) {
    return fs_dup(oldfd);
}

int dup2(int oldfd, int newfd) {
    return fs_dup2(oldfd, newfd);
}
