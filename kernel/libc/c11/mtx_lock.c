/* KallistiOS ##version##

   mtx_lock.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <threads.h>

int mtx_lock(mtx_t *mtx) {
    return mutex_lock(mtx) ? thrd_error : thrd_success;
}
