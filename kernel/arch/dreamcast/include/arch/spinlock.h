/* KallistiOS ##version##

   arch/dreamcast/include/spinlock.h
   Copyright (C) 2001 Megan Potter

*/

/** \file    arch/spinlock.h
    \brief   Simple locking.
    \ingroup kthreads

    This file contains definitions for very simple locks. Most of the time, you
    will probably not use such low-level locking, but will opt for something
    more fully featured like mutexes, semaphores, reader-writer semaphores, or
    recursive locks.

    \author Megan Potter

    \see    kos/sem.h
    \see    kos/mutex.h
    \see    kos/rwsem.h
    \see    kos/recursive_lock.h
*/

#ifndef __ARCH_SPINLOCK_H
#define __ARCH_SPINLOCK_H

/* Defines processor specific spinlocks */

#include <sys/cdefs.h>
__BEGIN_DECLS

/* DC implementation uses threads most of the time */
#include <kos/thread.h>

/** \brief  Spinlock data type. */
typedef volatile int spinlock_t;

/** \brief  Spinlock initializer.

    All created spinlocks should be initialized with this initializer so that
    they are in a sane state, ready to be used.
*/
#define SPINLOCK_INITIALIZER 0

/** \brief  Initialize a spinlock.

    This function-like macro abstracts initializing a spinlock, in case the
    initializer is not applicable to what you are doing.

    \param  A               A pointer to the spinlock to be initialized.
*/
#define spinlock_init(A) *(A) = SPINLOCK_INITIALIZER

/* Note here that even if threads aren't enabled, we'll still set the
   lock so that it can be used for anti-IRQ protection (e.g., malloc) */

/** \brief  Spin on a lock.

    This macro will spin on the lock, and will not return until the lock has
    been obtained for the calling thread.

    \param  A               A pointer to the spinlock to be locked.
*/
#define spinlock_lock(A) do { \
        spinlock_t * __lock = A; \
        int __gotlock = 0; \
        while(1) { \
            __asm__ __volatile__("tas.b @%1\n\t" \
                                 "movt %0\n\t" \
                                 : "=r" (__gotlock) \
                                 : "r" (__lock) \
                                 : "t", "memory"); \
            if(!__gotlock) \
                thd_pass(); \
            else break; \
        } \
    } while(0)

/** \brief  Try to lock, without spinning.

    This macro will attempt to lock the lock, but will not spin. Instead, it
    will return whether the lock was obtained or not.

    \param  A               A pointer to the spinlock to be locked.
    \return                 0 if the lock is held by another thread. Non-zero if
                            the lock was successfully obtained.
*/
#define spinlock_trylock(A) ({ \
        int __gotlock = 0; \
        do { \
            spinlock_t *__lock = A; \
            __asm__ __volatile__("tas.b @%1\n\t" \
                                 "movt %0\n\t" \
                                 : "=r" (__gotlock) \
                                 : "r" (__lock) \
                                 : "t", "memory"); \
        } while(0); \
        __gotlock; \
    })

/** \brief  Free a lock.

    This macro will unlock the lock that is currently held by the calling
    thread. Do not use this macro unless you actually hold the lock!

    \param  A               A pointer to the spinlock to be unlocked.
*/
#define spinlock_unlock(A) do { \
        *(A) = 0; \
    } while(0)

/** \brief  Determine if a lock is locked.

    This macro will return whether or not the lock specified is actually locked
    when it is called. This is NOT a thread-safe way of determining if a lock
    will be locked when you get around to locking it!

    \param  A               A pointer to the spinlock to be checked.
*/
#define spinlock_is_locked(A) ( *(A) != 0 )

__END_DECLS

#endif  /* __ARCH_SPINLOCK_H */
