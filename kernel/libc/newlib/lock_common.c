/* KallistiOS ##version##

   lock_common.c
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2024 Falco Girgis
   Copyright (C) 2025 Eric Fradella
*/

#include <assert.h>
#include <arch/irq.h>
#include <arch/spinlock.h>
#include <kos/thread.h>
#include <sys/lock.h>

void __newlib_lock_init(__newlib_lock_t *lock) {
    spinlock_init(lock);
}

void __newlib_lock_close(__newlib_lock_t *lock) {
    (void)lock;
}

void __newlib_lock_acquire(__newlib_lock_t *lock) {
    spinlock_lock(lock);
}

int __newlib_lock_try_acquire(__newlib_lock_t *lock) {
    return spinlock_trylock(lock);
}

void __newlib_lock_release(__newlib_lock_t *lock) {
    spinlock_unlock(lock);
}


void __newlib_lock_init_recursive(__newlib_recursive_lock_t *lock) {
    lock->owner = NULL;
    lock->nest = 0;
    spinlock_init(&lock->lock);
}

void __newlib_lock_close_recursive(__newlib_recursive_lock_t *lock) {
    /* Empty */
    (void)lock;
}

void __newlib_lock_acquire_recursive(__newlib_recursive_lock_t *lock) {
    int old;
    int iscur;

    // Check to see if we already own it. If so, everything is clear
    // to incr nest. Otherwise, we can safely go on to do a normal
    // spinlock wait.
    old = irq_disable();
    iscur = lock->owner == thd_current;
    irq_restore(old);

    if(iscur) {
        lock->nest++;
        return;
    }

    // It doesn't belong to us. Wait for it to come free.
    spinlock_lock(&lock->lock);

    // We own it now, so it's safe to init the rest of this.
    lock->owner = thd_current;
    lock->nest = 1;
}

/* Similar to __newlib_lock_acquire_recursive(), except that it can
   fail to obtain the lock. */
int __newlib_lock_try_acquire_recursive(__newlib_recursive_lock_t *lock) {
    if(lock->owner == thd_get_current()) {
        lock->nest++;
        return 1;
    }

    if(spinlock_trylock(&lock->lock)) {
        lock->owner = thd_get_current();
        lock->nest = 1;
        return 1;
    }

    return 0;
}

void __newlib_lock_release_recursive(__newlib_recursive_lock_t *lock) {
    // Check to see how much we own it.
    if(lock->nest == 1) {
        lock->owner = NULL;
        lock->nest = -1;
        spinlock_unlock(&lock->lock);
    }
    else {
        lock->nest--;
    }
}
