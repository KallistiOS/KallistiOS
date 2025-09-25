/* KallistiOS ##version##

   rwsem.c
   Copyright (C) 2008, 2012 Lawrence Sebald
   Copyright (C) 2025 Paul Cercueil
*/

/* Defines reader/writer semaphores */

#include <stdatomic.h>
#include <stdlib.h>
#include <errno.h>

#include <kos/rwsem.h>
#include <kos/timer.h>

typedef enum rwsem_update_type {
    UPDATE_TYPE_READ,
    UPDATE_TYPE_WRITE,
    UPDATE_TYPE_UPGRADE,
} rwsem_update_type_t;

int rwsem_init(rw_semaphore_t *s) {
    s->read_count = 0;
    s->write_lock = (mutex_t)MUTEX_INITIALIZER;
    s->read_lock = (mutex_t)MUTEX_INITIALIZER;

    return 0;
}

/* Destroy a reader/writer semaphore */
int rwsem_destroy(rw_semaphore_t *s) {
    if(mutex_is_locked(&s->write_lock) || mutex_is_locked(&s->read_lock)) {
        errno = EBUSY;
        return -1;
    }

    return 0;
}

static int rwsem_update_timed(rw_semaphore_t *s, int timeout,
                              rwsem_update_type_t type) {
    uint64_t deadline = 0;

    if(timeout)
        deadline = timer_ms_gettime64() + timeout;

    if(mutex_lock_timed(&s->write_lock, timeout))
        return -1;

    if(type != UPDATE_TYPE_READ || atomic_fetch_add(&s->read_count, 1) == 0) {
        if(timeout) {
            /* Update the timeout value to the remaining time */
            timeout = deadline - timer_ms_gettime64();
            if(timeout <= 0) {
                if(type == UPDATE_TYPE_READ)
                    atomic_fetch_sub(&s->read_count, 1);
                mutex_unlock(&s->write_lock);
                errno = ETIMEDOUT;
                return -1;
            }
        }

        if(type == UPDATE_TYPE_UPGRADE)
            rwsem_read_unlock(s);

        if(mutex_lock_timed(&s->read_lock, timeout)) {
            if(type == UPDATE_TYPE_READ) {
                atomic_fetch_sub(&s->read_count, 1);
            } else if(type == UPDATE_TYPE_UPGRADE
                      && atomic_fetch_add(&s->read_count, 1) == 0) {
                /* mutex_locked_timed() timed out, but the read count we just
                 * updated was zero, which means that whatever was holding up
                 * the mutex may have unlocked it since then, or will unlock it
                 * the next time it runs without delay. This is guaranteed
                 * because we hold up the write mutex, so no other reader or
                 * writer can lock up the read mutex before we do. */
                mutex_lock(&s->read_lock);
            }

            mutex_unlock(&s->write_lock);
            errno = ETIMEDOUT;
            return -1;
        }
    }

    if(type == UPDATE_TYPE_READ)
        mutex_unlock(&s->write_lock);

    return 0;
}

/* Lock a reader/writer semaphore for reading */
int rwsem_read_lock_timed(rw_semaphore_t *s, int timeout) {
    return rwsem_update_timed(s, timeout, UPDATE_TYPE_READ);
}

int rwsem_read_lock_irqsafe(rw_semaphore_t *s) {
    if(irq_inside_int())
        return rwsem_read_trylock(s);
    else
        return rwsem_read_lock(s);
}

/* Lock a reader/writer semaphore for writing */
int rwsem_write_lock_timed(rw_semaphore_t *s, int timeout) {
    return rwsem_update_timed(s, timeout, UPDATE_TYPE_WRITE);
}

int rwsem_write_lock_irqsafe(rw_semaphore_t *s) {
    if(irq_inside_int())
        return rwsem_write_trylock(s);
    else
        return rwsem_write_lock(s);
}

/* Unlock a reader/writer semaphore from a read lock. */
int rwsem_read_unlock(rw_semaphore_t *s) {
    if(atomic_fetch_sub(&s->read_count, 1) == 1)
        mutex_unlock(&s->read_lock);

    return 0;
}

/* Unlock a reader/writer semaphore from a write lock. */
int rwsem_write_unlock(rw_semaphore_t *s) {
    mutex_unlock(&s->read_lock);
    mutex_unlock(&s->write_lock);

    return 0;
}

int rwsem_unlock(rw_semaphore_t *s) {
    /* We have readers -> rwsem is a read lock. */
    if(s->read_count > 0)
        return rwsem_read_unlock(s);

    /* No readers -> it's a write lock. */
    return rwsem_write_unlock(s);
}

/* Attempt to lock a reader/writer semaphore for reading, but do not block. */
int rwsem_read_trylock(rw_semaphore_t *s) {
    if(mutex_trylock(&s->write_lock)) {
        errno = EWOULDBLOCK;
        return -1;
    }

    if(atomic_fetch_add(&s->read_count, 1) == 0
       && mutex_trylock(&s->read_lock)) {
        atomic_fetch_sub(&s->read_count, 1);
        mutex_unlock(&s->write_lock);
        errno = EWOULDBLOCK;
        return -1;
    }

    mutex_unlock(&s->write_lock);

    return 0;
}

/* Attempt to lock a reader/writer semaphore for writing, but do not block. */
int rwsem_write_trylock(rw_semaphore_t *s) {
    if(mutex_trylock(&s->write_lock)) {
        errno = EWOULDBLOCK;
        return -1;
    }

    if(mutex_trylock(&s->read_lock)) {
        mutex_unlock(&s->write_lock);
        errno = EWOULDBLOCK;
        return -1;
    }

    return 0;
}

/* "Upgrade" a read lock to a write lock. */
int rwsem_read_upgrade_timed(rw_semaphore_t *s, int timeout) {
    return rwsem_update_timed(s, timeout, UPDATE_TYPE_UPGRADE);
}

/* Attempt to upgrade a read lock to a write lock, but do not block. */
int rwsem_read_tryupgrade(rw_semaphore_t *s) {
    int one = 1;

    if(mutex_trylock(&s->write_lock)) {
        errno = EWOULDBLOCK;
        return -1;
    }

    if(!atomic_compare_exchange_strong(&s->read_count, &one, 0)) {
        /* upgrade failed */
        mutex_unlock(&s->write_lock);
        errno = EWOULDBLOCK;
        return -1;
    }

    return 0;
}

/* Return the current reader count */
int rwsem_read_count(const rw_semaphore_t *s) {
    return s->read_count;
}

/* Return the current status of the write lock */
int rwsem_write_locked(const rw_semaphore_t *s) {
    return mutex_is_locked(&s->write_lock);
}
