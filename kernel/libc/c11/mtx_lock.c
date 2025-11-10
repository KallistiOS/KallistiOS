/* KallistiOS ##version##

   mtx_lock.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <errno.h>
#include <threads.h>

int mtx_lock(mtx_t *mtx) {
    if(mtx->type > MUTEX_TYPE_RECURSIVE) {
        errno = EINVAL;
        return -1;
    }

    return mutex_lock(mtx);
}
