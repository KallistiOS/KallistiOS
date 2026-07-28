/* KallistiOS ##version##

   mtx_unlock.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <threads.h>

int mtx_unlock(mtx_t *mtx) {
    return mutex_unlock(mtx) ? thrd_error : thrd_success;
}
