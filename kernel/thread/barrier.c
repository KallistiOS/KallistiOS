/* KallistiOS ##version##

   barrier.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <arch/irq.h>
#include <kos/mutex.h>
#include <kos/cond.h>

/* We define the real barrier type in here to keep members private. We have to
   do this before including <kos/barrier.h>, otherwise it will define it without
   the internal type. */
#define __KTHREAD_HAVE_BARRIER_TYPE 1
#define THD_BARRIER_SIZE            64

/* Our actual barrier type, inspired by that of FreeBSD's libthr. */
struct barrier_int {
    mutex_t mutex;
    condvar_t cond;
    uint32_t pass;
    uint32_t count;
    uint32_t waiting;
    uint32_t refcnt;
    uint32_t cleanup;
};

typedef union kos_thd_barrier {
    struct barrier_int b;
    unsigned char __opaque[THD_BARRIER_SIZE];
    long int __align;
} thd_barrier_t;

#define STATIC_ASSERT(condition) \
    typedef char __CONCAT(_static_assert_, __LINE__)[ (condition) ? 1 : -1];

STATIC_ASSERT(sizeof(thd_barrier_t) == THD_BARRIER_SIZE)
#undef STATIC_ASSERT

#include <kos/barrier.h>

int thd_barrier_init(thd_barrier_t *__RESTRICT b, const void *__RESTRICT a,
                     unsigned count) {
    int tmp, rv;

    if(!b || a || !count || count > UINT32_MAX)
        return EINVAL;

    /* Initialize the barrier data. */
    memset(b, 0, sizeof(thd_barrier_t));
    tmp = errno;

    if(mutex_init(&b->b.mutex, MUTEX_TYPE_NORMAL)) {
        rv = errno;
        errno = tmp;
        return rv;
    }

    if(cond_init(&b->b.cond)) {
        rv = errno;
        errno = tmp;
        return rv;
    }

    b->b.count = (uint32_t)count;
    return 0;
}

int thd_barrier_destroy(thd_barrier_t *b) {
    int old;

    if(!b)
        return EINVAL;

    if(irq_inside_int())
        return EPERM;

    old = errno;

    /* The only way we should have issues locking this is if the barrier has
       already been cleaned up. */
    if(mutex_lock(&b->b.mutex)) {
        errno = old;
        return EINVAL;
    }

    /* Don't allow two cleanups or allow us to clean up if there's anyone
       actively waiting on the barrier. Technically, we could be lazy here and
       just break all the threads waiting, but that seems like a "bad idea". 
       Note that we do explicitly allow cleanup while we're unwinding a barrier
       that has been successfully reached by all threads, as it appears to be a
       common practice to have the serial thread clean up the barrier right
       after it gets back control. */
    if(b->b.cleanup || b->b.waiting) {
        mutex_unlock(&b->b.mutex);
        return EBUSY;
    }

    /* Start the cleanup in a controlled fashion. Note that we might have to
       deal with starting back up any threads that were waiting on the barrier,
       since we do allow this to be called while that unwinding is happening.
       However, we do not allow wait to be called on a barrier that is in a
       cleanup state, so we will never have to worry about other threads waiting
       on the barrier while we're cleaning it up. It seems that FreeBSD does
       allow this for some reason, so I may reevaluate this decision later... */
    b->b.cleanup = 1;

    for(;;) {
        if(b->b.refcnt)
            cond_wait(&b->b.cond, &b->b.mutex);
        else
            break;
    }

    /* Once we're out of the loop above, there's nobody still waiting on the 
       condvar in thd_barrier_wait(), and everyone should have cleared out from
       the function... Now we can actually clean everything up. Poison what we
       had for the state before just to make sure nobody tries to use the
       cleaned up barrier. */
    b->b.pass = UINT32_MAX;
    b->b.count = 0;
    b->b.waiting = UINT32_MAX;
    b->b.refcnt = UINT32_MAX;
    b->b.cleanup = UINT32_MAX;

    /* Clean up the mutex with IRQs disabled so that nobody can jump in and try
       to use our poisoned barrier before we destroy the mutex. */
    old = irq_disable();
    cond_destroy(&b->b.cond);
    mutex_unlock(&b->b.mutex);
    mutex_destroy(&b->b.mutex);
    irq_restore(old);

    return 0;
}

int thd_barrier_wait(thd_barrier_t *b) {
    int tmp;

    if(!b)
        return EINVAL;

    if(irq_inside_int())
        return EPERM;

    tmp = errno;

    /* The only way this should fail is if the barrier is already cleaned up. */
    if(mutex_lock(&b->b.mutex)) {
        errno = tmp;
        return EINVAL;
    }

    /* Make sure we aren't cleaning up the barrier right now. */
    if(b->b.cleanup || !b->b.count) {
        mutex_unlock(&b->b.mutex);
        return EINVAL;
    }

    /* Increment the wait count and see if we've had the correct number of
       threads wait. */
    ++b->b.waiting;

    if(b->b.waiting == b->b.count) {
        /* We've hit the wait count. Schedule all the sleeping threads to wake
           up soon. */
        b->b.waiting = 0;
        ++b->b.pass;
        cond_broadcast(&b->b.cond);
        mutex_unlock(&b->b.mutex);
        return THD_BARRIER_SERIAL_THREAD;
    }
    else {
        /* We're not the last thread needed, so it's time to sleep. The pass
           variable is used to save us from spurious wakeups on the condvar,
           since it is incremented in the barrier when the last thread calls
           this function. */
        uint32_t pass = b->b.pass;

        ++b->b.refcnt;

        do {
            cond_wait(&b->b.cond, &b->b.mutex);
        } while(b->b.pass == pass);

        --b->b.refcnt;

        /* If we're cleaning up and this was the last thread to get the signal
           to wake up from the wait, make sure that the thread cleaning up gets
           the signal to complete the cleanup once we exit the critical
           section. */
        if(b->b.cleanup && b->b.refcnt == 0)
            cond_broadcast(&b->b.cond);

        mutex_unlock(&b->b.mutex);
        return 0;
    }
}
