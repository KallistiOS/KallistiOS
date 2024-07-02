/* KallistiOS ##version##

   mutex.c
   Copyright (C) 2012, 2015 Lawrence Sebald

*/

#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include <kos/mutex.h>
#include <kos/genwait.h>
#include <kos/dbglog.h>

#include <arch/irq.h>
#include <arch/timer.h>

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
    int rv = 0, old;

    old = irq_disable();

    if(m->type < MUTEX_TYPE_NORMAL || m->type > MUTEX_TYPE_RECURSIVE) {
        errno = EINVAL;
        rv = -1;
    }
    else if(m->count) {
        /* Send an error if its busy */
        errno = EBUSY;
        rv = -1;
    }
    else {
        /* Set it to an invalid type of mutex */
        m->type = -1;
    }

    /* If the mutex was created with the deprecated mutex_create(), free it. */
    if(m->dynamic) {
        free(m);
    }

    irq_restore(old);
    return rv;
}

int mutex_lock(mutex_t *m) {
    return mutex_lock_timed(m, 0);
}

int mutex_lock_timed(mutex_t *m, int timeout) {
    uint64_t deadline = 0;
    int old, rv = 0;

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

    old = irq_disable();

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

    irq_restore(old);
    return rv;
}

int mutex_is_locked(mutex_t *m) {
    return !!m->count;
}

int mutex_trylock(mutex_t *m) {
    int old, rv = 0;
    kthread_t *thd = thd_current;

    old = irq_disable();

    /* If we're inside of an interrupt, pick a special value for the thread that
       would otherwise be impossible... */
    if(irq_inside_int())
        thd = (kthread_t *)0xFFFFFFFF;

    if(m->type < MUTEX_TYPE_NORMAL || m->type > MUTEX_TYPE_RECURSIVE) {
        errno = EINVAL;
        rv = -1;
    }
    /* Check if the lock is held by some other thread already */
    else if(m->count && m->holder != thd) {
        errno = EAGAIN;
        rv = -1;
    }
    else {
        m->holder = thd;

        switch(m->type) {
            case MUTEX_TYPE_NORMAL:
            case MUTEX_TYPE_OLDNORMAL:
            case MUTEX_TYPE_ERRORCHECK:
                if(m->count) {
                    errno = EDEADLK;
                    rv = -1;
                }
                else {
                    m->count = 1;
                }
                break;

            case MUTEX_TYPE_RECURSIVE:
                if(m->count == INT_MAX) {
                    errno = EAGAIN;
                    rv = -1;
                }
                else {
                    ++m->count;
                }
                break;
        }
    }

    irq_restore(old);
    return rv;
}

static int mutex_unlock_common(mutex_t *m, kthread_t *thd) {
    int old, rv = 0, wakeup = 0;

    old = irq_disable();

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
                rv = -1;
            }
            else {
                m->count = 0;
                m->holder = NULL;
                wakeup = 1;
            }
            break;

        case MUTEX_TYPE_RECURSIVE:
            if(m->holder != thd) {
                errno = EPERM;
                rv = -1;
            }
            else if(!--m->count) {
                m->holder = NULL;
                wakeup = 1;
            }
            break;

        default:
            errno = EINVAL;
            rv = -1;
    }

    /* If we need to wake up a thread, do so. */
    if(wakeup)
        genwait_wake_one(m);

    irq_restore(old);
    return rv;
}

int mutex_unlock(mutex_t *m) {
    kthread_t *thd = thd_current;

    /* If we're inside of an interrupt, use the special value for the thread
       from mutex_trylock(). */
    if(irq_inside_int())
        thd = (kthread_t *)0xFFFFFFFF;

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
