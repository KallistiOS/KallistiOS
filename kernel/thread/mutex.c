/* KallistiOS ##version##

   mutex.c
   Copyright (C) 2012, 2015 Lawrence Sebald
   Copyright (C) 2024 Paul Cercueil

*/

#include <assert.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include <kos/mutex.h>
#include <kos/genwait.h>
#include <kos/dbglog.h>

#include <arch/irq.h>
#include <kos/timer.h>

/* Thread pseudo-ptr representing an active IRQ context. */
#define IRQ_THREAD  ((kthread_t *)0xFFFFFFFF)

static int mutex_trylock_thd(mutex_t *m, kthread_t *thd);

int mutex_init(mutex_t *m, unsigned int mtype) {
    /* Check the type */
    if(mtype > MUTEX_TYPE_RECURSIVE) {
        errno = EINVAL;
        return -1;
    }

    /* Set it up */
    m->type = mtype;
    m->holder = NULL;
    m->count = 0;

    return 0;
}

int mutex_destroy(mutex_t *m) {
    irq_disable_scoped();

    if(m->type > MUTEX_TYPE_RECURSIVE) {
        errno = EINVAL;
        return -1;
    }

    if(m->count) {
        /* Send an error if its busy */
        errno = EBUSY;
        return -1;
    }

    /* Set it to an invalid type of mutex */
    m->type = MUTEX_TYPE_DESTROYED;

    return 0;
}

int mutex_lock_irqsafe(mutex_t *m) {
    if(irq_inside_int())
        return mutex_trylock_thd(m, IRQ_THREAD);
    else
        return mutex_lock(m);
}

int mutex_lock_timed(mutex_t *m, unsigned int timeout) {
    uint64_t deadline = 0;
    int rv = 0;

    assert(!irq_inside_int()); /* Only usable outside IRQ handlers */

    rv = mutex_trylock_thd(m, thd_current);
    if(!rv || errno != EBUSY)
        return rv;

    irq_disable_scoped();

    if(!m->holder) {
        m->count = 1;
        m->holder = thd_current;
        rv = 0;
    }
    else {
        if(timeout)
            deadline = timer_ms_gettime64() + timeout;

        for(;;) {
            /* Check whether we should boost priority. */
            if (m->holder->prio >= thd_current->prio) {
                m->holder->prio = thd_current->prio;

                /* Reschedule if currently scheduled. */
                if(m->holder->state == STATE_READY) {
                    /* Thread list is sorted by priority, update the position
                     * of the thread holding the lock */
                    thd_remove_from_runnable(m->holder);
                    thd_add_to_runnable(m->holder, true);
                }
            }

            rv = genwait_wait(m, timeout ? "mutex_lock_timed" : "mutex_lock",
                              timeout, NULL);
            if(rv < 0) {
                errno = ETIMEDOUT;
                break;
            }

            if(!m->holder) {
                m->holder = thd_current;
                m->count = 1;
                break;
            }

            if(timeout) {
                timeout = deadline - timer_ms_gettime64();
                if((int)timeout <= 0) {
                    errno = ETIMEDOUT;
                    rv = -1;
                    break;
                }
            }
        }
    }

    return rv;
}

int __pure mutex_is_locked(const mutex_t *m) {
    return !!m->holder;
}

int mutex_trylock(mutex_t *m) {
    kthread_t *thd = thd_current;

    /* If we're inside of an interrupt, pick a special value for the thread that
       would otherwise be impossible... */
    if(irq_inside_int())
        thd = IRQ_THREAD;

    return mutex_trylock_thd(m, thd);
}

static int mutex_trylock_thd(mutex_t *m, kthread_t *thd) {
    kthread_t *previous_thd = NULL;

    assert(m->type <= MUTEX_TYPE_RECURSIVE);

    if(atomic_compare_exchange_strong(&m->holder, &previous_thd, thd)) {
        m->count = 1;
        return 0;
    }

    if(previous_thd == thd) {
        assert(m->type == MUTEX_TYPE_RECURSIVE);

        /* Recursive mutex, we can just increment normally. */
        if(m->count == INT_MAX) {
            errno = EAGAIN;
            return -1;
        }

        ++m->count;
        return 0;
    }

    /* We did not get the lock */
    errno = EBUSY;
    return -1;
}

int mutex_unlock(mutex_t *m) {
    kthread_t *thd = thd_current;

    assert(m->type <= MUTEX_TYPE_RECURSIVE);

    /* If we're inside of an interrupt, use the special value for the thread
       from mutex_trylock(). */
    if(irq_inside_int())
        thd = IRQ_THREAD;

    assert(m->holder == thd && m->count > 0);

    if (!--m->count) {
        m->holder = NULL;

        /* Restore real priority in case we were dynamically boosted. */
        if (thd != IRQ_THREAD)
            thd->prio = thd->real_prio;

        /* If we need to wake up a thread, do so. */
        genwait_wake_one(m);
    }

    return 0;
}
