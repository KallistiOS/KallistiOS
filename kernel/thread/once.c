/* KallistiOS ##version##

   once.c
   Copyright (C) 2009, 2023 Lawrence Sebald
*/

#include <errno.h>

#include <kos/once.h>
#include <kos/mutex.h>
#include <kos/cond.h>

/* The lock used to make sure multiple threads don't try to run the same routine
   at the same time. */
static mutex_t lock = MUTEX_INITIALIZER;
static condvar_t cond = COND_INITIALIZER;

#define ONCE_COMPLETE   1
#define ONCE_INPROGRESS -1

int kthread_once(kthread_once_t *once_control, void (*init_routine)(void)) {
    if(!once_control) {
        errno = EINVAL;
        return -1;
    }

    /* Lock the lock. */
    if(mutex_lock(&lock)) {
        return -1;
    }

    /* If the function has already been run, unlock the lock and return. */
    if(*once_control == ONCE_COMPLETE) {
        mutex_unlock(&lock);
        return 0;
    }

    /* If the function is in progress in another thread, wait for it to finish.
       We share one mutex/condvar across all once controls, so we have to do
       this in a loop to prevent spurious wakeups. */
    if(*once_control == ONCE_INPROGRESS) {
        while(*once_control == ONCE_INPROGRESS) {
            if(cond_wait(&cond, &lock)) {
                mutex_unlock(&lock);
                return -1;
            }
        }

        /* We'll only get here once the once control has been set to 1, so
           return success. */
        mutex_unlock(&lock);
        return 0;
    }

    /* Set that we're in progress and unlock the lock (so other once controls
       can be used). Once that's done, run the function, re-obtain the lock,
       set that the function has been run and wake all threads waiting on this
       particular once control. */
    *once_control = ONCE_INPROGRESS;
    mutex_unlock(&lock);

    init_routine();

    mutex_lock(&lock);
    *once_control = ONCE_COMPLETE;
    cond_broadcast(&cond);
    mutex_unlock(&lock);

    return 0;
}
