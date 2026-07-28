/* KallistiOS ##version##

   cnd_wait.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <threads.h>

int cnd_wait(cnd_t *cond, mtx_t *mtx) {
    return cond_wait(cond, mtx) ? thrd_error : thrd_success;
}
