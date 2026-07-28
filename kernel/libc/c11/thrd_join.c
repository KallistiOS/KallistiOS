/* KallistiOS ##version##

   thrd_join.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <threads.h>
#include <stdint.h>

int thrd_join(thrd_t thr, int *res) {
    void *rv;

    if(thd_join(thr, &rv))
        return thrd_error;

    if(res)
        *res = (int)(intptr_t)rv;

    return thrd_success;
}
