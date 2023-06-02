/* KallistiOS ##version##

   recursive_lock.c
   Copyright (C) 2008, 2009, 2012 Lawrence Sebald
*/

/* This file defines recursive locks. */

#include <stdlib.h>
#include <errno.h>

#include <kos/recursive_lock.h>

/* Create a recursive lock */
recursive_lock_t *rlock_create(void) {
    recursive_lock_t *rv;

    if(!(rv = (recursive_lock_t *)malloc(sizeof(recursive_lock_t)))) {
        errno = ENOMEM;
        return NULL;
    }

    /* Init the mutex */
    mutex_init(rv, MUTEX_TYPE_RECURSIVE);
    rv->dynamic = 1;

    return rv;
}

/* Destroy a recursive lock */
void rlock_destroy(recursive_lock_t *l) {
    mutex_destroy(l);
}

/* Lock a recursive lock */
int rlock_lock(recursive_lock_t *l) {
    return mutex_lock_timed(l, 0);
}

/* Lock a recursive lock, with timeout (in milliseconds) */
int rlock_lock_timed(recursive_lock_t *l, int timeout) {
    return mutex_lock_timed(l, timeout);
}

/* Unlock a recursive lock */
int rlock_unlock(recursive_lock_t *l) {
    return mutex_unlock(l);
}

/* Attempt to lock a recursive lock */
int rlock_trylock(recursive_lock_t *l) {
    return mutex_trylock(l);
}

/* Return whether or not the lock is currently held */
int rlock_is_locked(recursive_lock_t *l) {
    return mutex_is_locked(l);
}
