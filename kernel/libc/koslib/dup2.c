/* KallistiOS ##version##

   dup2.c
   Copyright (C) 2024 Andress Barajas

*/

#include <unistd.h>
#include <kos/fs.h>

int dup2(int oldfd, int newfd) {
    return fs_dup2(oldfd, newfd);
}
