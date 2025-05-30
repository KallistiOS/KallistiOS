/* KallistiOS ##version##

   cond.c
   Copyright (C) 2001, 2003 Megan Potter
   Copyright (C) 2012 Lawrence Sebald
*/

/* Defines condition variables, which are like semaphores that automatically
   signal all waiting processes when a signal() is called. */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include <kos/thread.h>
#include <kos/cond.h>
#include <kos/genwait.h>

#include <kos/dbglog.h>

/**************************************/

int cond_init(condvar_t *cv) {
    cv->dummy = 0;
    return 0;
}

/* Free a condvar */
int cond_destroy(condvar_t *cv) {
    /* Give all sleeping threads a timed out error */
    genwait_wake_all_err(cv, ENOTRECOVERABLE);

    return 0;
}

int cond_wait_timed(condvar_t *cv, mutex_t *m, int timeout) {
    int rv;

    if(irq_inside_int()) {
        dbglog(DBG_WARNING, "cond_wait: called inside interrupt\n");
        errno = EPERM;
        return -1;
    }

    irq_disable_scoped();

    if(m->type > MUTEX_TYPE_RECURSIVE ||
       !mutex_is_locked(m)) {
        errno = EINVAL;
        return -1;
    }

    /* First of all, release the associated mutex */
    mutex_unlock(m);

    /* Now block us until we're signaled */
    rv = genwait_wait(cv, timeout ? "cond_wait_timed" : "cond_wait", timeout,
                      NULL);

    if(rv < 0 && errno == EAGAIN)
        errno = ETIMEDOUT;

    /* Re-lock our mutex */
    mutex_lock(m);

    return rv;
}

int cond_wait(condvar_t *cv, mutex_t *m) {
    return cond_wait_timed(cv, m, 0);
}

int cond_signal(condvar_t *cv) {
    irq_disable_scoped();

    /* Wake one thread who's waiting, if any */
    genwait_wake_one(cv);

    return 0;
}

int cond_broadcast(condvar_t *cv) {
    irq_disable_scoped();

    /* Wake all threads who are waiting */
    genwait_wake_all(cv);

    return 0;
}
