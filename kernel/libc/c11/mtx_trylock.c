/* KallistiOS ##version##

   mtx_trylock.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <threads.h>
#include <errno.h>

int mtx_trylock(mtx_t *mtx) {
    if(mutex_trylock(mtx)) {
        if(errno == EBUSY)
            return thrd_busy;

        return thrd_error;
    }

    return thrd_success;
}
