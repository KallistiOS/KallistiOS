/* KallistiOS ##version##

   mutex.c
   Copyright (C) 2012, 2015 Lawrence Sebald
   Copyright (C) 2024 Paul Cercueil

*/

#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include <kos/mutex.h>
#include <kos/genwait.h>
#include <kos/dbglog.h>

#include <arch/irq.h>
#include <arch/timer.h>

/* Thread pseudo-ptr representing an active IRQ context. */
#define IRQ_THREAD  ((kthread_t *)0xFFFFFFFF)

mutex_t *mutex_create(void) {
    mutex_t *rv;

    dbglog(DBG_WARNING, "Creating mutex with deprecated mutex_create(). Please "
           "update your code!\n");

    if(!(rv = (mutex_t *)malloc(sizeof(mutex_t)))) {
        errno = ENOMEM;
        return NULL;
    }

    rv->type = MUTEX_TYPE_NORMAL;
    rv->dynamic = 1;
    rv->holder = NULL;
    rv->count = 0;

    return rv;
}

int mutex_init(mutex_t *m, int mtype) {
    /* Check the type */
    if(mtype < MUTEX_TYPE_NORMAL || mtype > MUTEX_TYPE_RECURSIVE) {
        errno = EINVAL;
        return -1;
    }

    /* Set it up */
    m->type = mtype;
    m->dynamic = 0;
    m->holder = NULL;
    m->count = 0;

    return 0;
}

int mutex_destroy(mutex_t *m) {
    irq_disable_scoped();

    if(m->type < MUTEX_TYPE_NORMAL || m->type > MUTEX_TYPE_RECURSIVE) {
        errno = EINVAL;
        return -1;
    }

    if(m->count) {
        /* Send an error if its busy */
        errno = EBUSY;
        return -1;
    }

    /* Set it to an invalid type of mutex */
    m->type = -1;

    /* If the mutex was created with the deprecated mutex_create(), free it. */
    if(m->dynamic) {
        free(m);
    }

    return 0;
}

int mutex_lock(mutex_t *m) {
    return mutex_lock_timed(m, 0);
}

int mutex_lock_irqsafe(mutex_t *m) {
    if(irq_inside_int())
        return mutex_trylock(m);
    else
        return mutex_lock(m);
}

int mutex_lock_timed(mutex_t *m, int timeout) {
    uint64_t deadline = 0;
    int rv = 0;

    if((rv = irq_inside_int())) {
        dbglog(DBG_WARNING, "%s: called inside an interrupt with code: "
               "%x evt: %.4x\n",
               timeout ? "mutex_lock_timed" : "mutex_lock",
               ((rv >> 16) & 0xf), (rv & 0xffff));
        errno = EPERM;
        return -1;
    }

    if(timeout < 0) {
        errno = EINVAL;
        return -1;
    }

    irq_disable_scoped();

    if(m->type < MUTEX_TYPE_NORMAL || m->type > MUTEX_TYPE_RECURSIVE) {
        errno = EINVAL;
        rv = -1;
    }
    else if(!m->count) {
        m->count = 1;
        m->holder = thd_current;
    }
    else if(m->type == MUTEX_TYPE_RECURSIVE && m->holder == thd_current) {
        if(m->count == INT_MAX) {
            errno = EAGAIN;
            rv = -1;
        }
        else {
            ++m->count;
        }
    }
    else if(m->type == MUTEX_TYPE_ERRORCHECK && m->holder == thd_current) {
        errno = EDEADLK;
        rv = -1;
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
                if(timeout <= 0) {
                    errno = ETIMEDOUT;
                    rv = -1;
                    break;
                }
            }
        }
    }

    return rv;
}

int mutex_is_locked(mutex_t *m) {
    return !!m->count;
}

int mutex_trylock(mutex_t *m) {
    kthread_t *thd = thd_current;

    irq_disable_scoped();

    /* If we're inside of an interrupt, pick a special value for the thread that
       would otherwise be impossible... */
    if(irq_inside_int())
        thd = IRQ_THREAD;

    if(m->type < MUTEX_TYPE_NORMAL || m->type > MUTEX_TYPE_RECURSIVE) {
        errno = EINVAL;
        return -1;
    }

    /* Check if the lock is held by some other thread already */
    if(m->count && m->holder != thd) {
        errno = EAGAIN;
        return -1;
    }

    m->holder = thd;

    switch(m->type) {
        case MUTEX_TYPE_NORMAL:
        case MUTEX_TYPE_OLDNORMAL:
        case MUTEX_TYPE_ERRORCHECK:
            if(m->count) {
                errno = EDEADLK;
                return -1;
            }

            m->count = 1;
            break;

        case MUTEX_TYPE_RECURSIVE:
            if(m->count == INT_MAX) {
                errno = EAGAIN;
                return -1;
            }

            ++m->count;
            break;
    }

    return 0;
}

static int mutex_unlock_common(mutex_t *m, kthread_t *thd) {
    int wakeup = 0;

    irq_disable_scoped();

    switch(m->type) {
        case MUTEX_TYPE_NORMAL:
        case MUTEX_TYPE_OLDNORMAL:
            m->count = 0;
            m->holder = NULL;
            wakeup = 1;
            break;

        case MUTEX_TYPE_ERRORCHECK:
            if(m->holder != thd) {
                errno = EPERM;
                return -1;
            }

            m->count = 0;
            m->holder = NULL;
            wakeup = 1;
            break;

        case MUTEX_TYPE_RECURSIVE:
            if(m->holder != thd) {
                errno = EPERM;
                return -1;
            }

            if(!--m->count) {
                m->holder = NULL;
                wakeup = 1;
            }
            break;

        default:
            errno = EINVAL;
            return -1;
    }

    /* If we need to wake up a thread, do so. */
    if(wakeup) {
        /* Restore real priority in case we were dynamically boosted. */
        if (thd != IRQ_THREAD)
            thd->prio = thd->real_prio;

        genwait_wake_one(m);
    }

    return 0;
}

int mutex_unlock(mutex_t *m) {
    kthread_t *thd = thd_current;

    /* If we're inside of an interrupt, use the special value for the thread
       from mutex_trylock(). */
    if(irq_inside_int())
        thd = IRQ_THREAD;

    return mutex_unlock_common(m, thd);
}

int mutex_unlock_as_thread(mutex_t *m, kthread_t *thd) {
    /* Make sure we're in an IRQ handler */
    if(!irq_inside_int()) {
        errno = EACCES;
        return -1;
    }

    return mutex_unlock_common(m, thd);
}
