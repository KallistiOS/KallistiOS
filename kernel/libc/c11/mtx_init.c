/* KallistiOS ##version##

   mtx_init.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <threads.h>

int mtx_init(mtx_t *mtx, int type) {
    if(type & mtx_recursive)
        return mutex_init(mtx, MUTEX_TYPE_RECURSIVE) ? thrd_error : thrd_success;

    return mutex_init(mtx, MUTEX_TYPE_NORMAL) ? thrd_error : thrd_success;
}
