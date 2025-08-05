/* KallistiOS ##version##

   sem.c
   Copyright (C) 2001, 2002, 2003 Megan Potter
   Copyright (C) 2012, 2020 Lawrence Sebald
*/

/* Defines semaphores */

/**************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include <kos/thread.h>
#include <kos/sem.h>
#include <kos/genwait.h>
#include <kos/dbglog.h>

/**************************************/

int sem_init(semaphore_t *sm, int count) {
    if(!sm) {
        errno = EFAULT;
        return -1;
    }
    else if(count < 0) {
        sm->initialized = 0;
        errno = EINVAL;
        return -1;
    }

    sm->count = count;
    sm->initialized = 1;
    return 0;
}

/* Take care of destroying a semaphore */
int sem_destroy(semaphore_t *sm) {
    /* Wake up any queued threads with an error */
    genwait_wake_all_err(sm, ENOTRECOVERABLE);

    sm->count = 0;
    sm->initialized = 0;

    return 0;
}

/* Wait on a semaphore, with timeout (in milliseconds) */
int sem_wait_timed(semaphore_t *sm, int timeout) {
    int rv = 0;

    /* Make sure we're not inside an interrupt */
    if((rv = irq_inside_int())) {
        dbglog(DBG_WARNING, "%s: called inside an interrupt with code: %x evt: %.4x\n",
               timeout ? "sem_wait_timed" : "sem_wait",
               ((rv>>16) & 0xf), (rv & 0xffff));
        errno = EPERM;
        return -1;
    }

    /* Check for smarty clients */
    if(timeout < 0) {
        errno = EINVAL;
        return -1;
    }

    /* Disable interrupts */
    irq_disable_scoped();

    if(sm->initialized != 1) {
        errno = EINVAL;
        rv = -1;
    }
    /* If there's enough count left, then let the thread proceed */
    else if(sm->count > 0) {
        sm->count--;
    }
    else {
        /* Block us until we're signaled */
        sm->count--;
        rv = genwait_wait(sm, timeout ? "sem_wait_timed" : "sem_wait", timeout,
                          NULL);

        /* Did we fail to get the lock? */
        if(rv < 0) {
            rv = -1;
            ++sm->count;

            if(errno == EAGAIN)
                errno = ETIMEDOUT;
        }
    }

    return rv;
}

int sem_wait(semaphore_t *sm) {
    return sem_wait_timed(sm, 0);
}

/* Attempt to wait on a semaphore. If the semaphore would block,
   then return an error instead of actually blocking. */
int sem_trywait(semaphore_t *sm) {
    int rv = 0;

    irq_disable_scoped();

    if(sm->initialized != 1) {
        errno = EINVAL;
        rv = -1;
    }
    /* Is there enough count left? */
    else if(sm->count > 0) {
        sm->count--;
    }
    else {
        rv = -1;
        errno = EWOULDBLOCK;
    }

    return rv;
}

/* Signal a semaphore */
int sem_signal(semaphore_t *sm) {
    int woken, rv = 0;

    irq_disable_scoped();

    if(sm->initialized != 1) {
        errno = EINVAL;
        rv = -1;
    }
    /* Is there anyone waiting? If so, pass off to them */
    else if(sm->count < 0) {
        woken = genwait_wake_cnt(sm, 1, 0);
        (void)woken;
        assert(woken == 1);
        sm->count++;
    }
    else {
        /* No one is waiting, so just add another tick */
        sm->count++;
    }

    return rv;
}

/* Return the semaphore count */
int sem_count(const semaphore_t *sm) {
    /* Look for the semaphore */
    return sm->count;
}
