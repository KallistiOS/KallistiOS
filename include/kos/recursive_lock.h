/* KallistiOS ##version##

   include/kos/recursive_lock.h
   Copyright (C) 2008, 2010, 2012 Lawrence Sebald

*/

/** \file    kos/recursive_lock.h
    \brief   Definitions for a recursive mutex.
    \ingroup kthreads

    This file defines a recursive lock mechanism, similar to a mutex, but that a
    single thread can obtain as many times as it wants. A single thread is still
    limited to holding the lock at a time, but it can hold it an "unlimited"
    number of times (actually limited to INT_MAX, but who's counting).

    \deprecated
    These are now just wrappers around the MUTEX_TYPE_RECURSIVE that is now
    provided and will be removed at some point in the future. Please update your
    code to use that type instead.

    \author Lawrence Sebald
*/

#ifndef __KOS_RECURSIVE_LOCK_H
#define __KOS_RECURSIVE_LOCK_H

#include <kos/cdefs.h>

__BEGIN_DECLS

#include <kos/mutex.h>

/** \brief  Recursive lock structure.

    Recursive locks are just a simple wrapper around mutexes at this point. You
    should not use this type in any new code.

    \headerfile kos/recursive_lock.h
*/
typedef mutex_t recursive_lock_t;

/** \brief  Allocate a new recursive lock.

    \deprecated
    This function allocates a new recursive lock that is initially not locked.

    \return The created lock, or NULL on failure (errno will be set to ENOMEM to
            indicate that the system appears to be out of memory).
*/
recursive_lock_t *rlock_create(void) __depr("Use mutexes instead.");

/** \brief  Destroy a recursive lock.

    \deprecated
    This function cleans up a recursive lock. It is an error to attempt to
    destroy a locked recursive lock.

    \param  l       The recursive lock to destroy. It must be unlocked.
*/
void rlock_destroy(recursive_lock_t *l) __depr("Use mutexes instead.");

/** \brief  Lock a recursive lock.
    
    \deprecated
    This function attempts to lock the requested lock, and if it cannot it will
    block until that is possible.

    \param  l       The recursive lock to lock.
    \retval -1      On error, errno will be set to EPERM if this function is
                    called inside an interrupt, or EINTR if it is interrupted.
    \retval 0       On success.
    \sa rlock_trylock
    \sa rlock_lock_timed
*/
int rlock_lock(recursive_lock_t *l) __depr("Use mutexes instead.");

/** \brief  Lock a recursive lock (with a timeout).

    \deprecated
    This function attempts to lock the requested lock, and if it cannot it will
    block until either it is possible to acquire the lock or timeout
    milliseconds have elapsed.

    \param  l       The recursive lock to lock.
    \param  timeout The maximum number of milliseconds to wait. 0 is an
                    unlimited timeout (equivalent to rlock_lock).
    \retval -1      On error, errno will be set to EPERM if this function is
                    called inside an interrupt, EINTR if the function is
                    interrupted, or EAGAIN if the timeout expires.
    \retval 0       On success.
    \sa rlock_trylock
    \sa rlock_lock_timed
*/
int rlock_lock_timed(recursive_lock_t *l, int timeout)
    __depr("Use mutexes instead.");

/** \brief  Unlock a recursive lock.

    \deprecated
    This function releases the lock one time from the current thread.

    \param  l       The recursive lock to unlock.
    \retval -1      On error, errno will be set to EPERM if the lock is not held
                    by the calling thread.
    \retval 0       On success.
*/
int rlock_unlock(recursive_lock_t *l) __depr("Use mutexes instead.");

/** \brief  Attempt to lock a recursive lock without blocking.

    \deprecated
    This function attempts to lock a recursive lock without blocking. This
    function, unlike rlock_lock and rlock_lock_timed is safe to call inside an
    interrupt.

    \param  l       The recursive lock to lock.
    \retval -1      On error, errno will be set to EWOULDBLOCK if the lock is
                    currently held by another thread.
    \retval 0       On success.
    \sa rlock_lock
    \sa rlock_lock_timed
*/
int rlock_trylock(recursive_lock_t *l) __depr("Use mutexes instead.");

/** \brief  Check if a recursive lock is currently held by any thread.
    
    \deprecated
    This function checks whether or not a lock is currently held by any thread,
    including the calling thread. Note that this is <b>NOT</b> a safe way to
    check if a lock <em>will</em> be held by the time you get around to locking
    it.

    \retval TRUE    If the lock is held by any thread.
    \retval FALSE   If the lock is not currently held by any thread.
*/
int rlock_is_locked(recursive_lock_t *l) __depr("Use mutexes instead.");

__END_DECLS

#endif /* !__KOS_RECURSIVE_LOCK_H */
