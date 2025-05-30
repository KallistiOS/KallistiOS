/* KallistiOS ##version##

   include/kos/mutex.h
   Copyright (C) 2001, 2003 Megan Potter
   Copyright (C) 2012, 2015 Lawrence Sebald

*/

/** \file   kos/mutex.h
    \brief  Mutual exclusion locks.
    \ingroup kthreads

    This file defines mutual exclusion locks (or mutexes for short). The concept
    of a mutex is one of the most common types of locks in a multi-threaded
    environment. Mutexes do exactly what they sound like, they keep two (or
    more) threads mutually exclusive from one another. A mutex is used around
    a block of code to prevent two threads from interfering with one another
    when only one would be appropriate to be in the block at a time.

    KallistiOS implements 3 types of mutexes, to bring it roughly in-line with
    POSIX. The types of mutexes that can be made are normal, error-checking, and
    recursive. Each has its own strengths and weaknesses, which are briefly
    discussed below.

    A normal mutex (MUTEX_TYPE_NORMAL) is the fastest and simplest mutex of the
    bunch. This is roughly equivalent to a semaphore that has been initialized
    with a count of 1. There is no protection against threads unlocking normal
    mutexes they didn't lock, nor is there any protection against deadlocks that
    would arise from locking the mutex twice.

    An error-checking mutex (MUTEX_TYPE_ERRORCHECK) adds a small amount of error
    checking on top of a normal mutex. This type will not allow you to lock the
    mutex twice (it will return an error if the same thread tries to lock it two
    times so it will not deadlock), and it will not allow a different thread to
    unlock the mutex if it isn't the one holding the lock.

    A recursive mutex (MUTEX_TYPE_RECURSIVE) extends the error checking mutex
    by allowing you to lock the mutex twice in the same thread, but you must
    also unlock it twice (this works for any number of locks -- lock it n times,
    you must unlock it n times). Still only one thread can hold the lock, but it
    may hold it as many times as it needs to. This is equivalent to the
    recursive_lock_t type that was available in KallistiOS for a while (before
    it was basically merged back into a normal mutex).

    There is a fourth type of mutex defined (MUTEX_TYPE_DEFAULT), which maps to
    the MUTEX_TYPE_NORMAL type. This is simply for alignment with POSIX.

    \author Lawrence Sebald
    \see    kos/sem.h
*/

#ifndef __KOS_MUTEX_H
#define __KOS_MUTEX_H

#include <kos/cdefs.h>

__BEGIN_DECLS

#include <kos/thread.h>

/** \brief  Mutual exclusion lock type.

    All members of this structure should be considered to be private. It is
    unsafe to change anything in here yourself.

    \headerfile kos/mutex.h
*/
typedef struct kos_mutex {
    int type;
    kthread_t *holder;
    int count;
} mutex_t;

/** \name  Mutex types
    \brief Types of Mutexes supported by KOS

    The values defined in here are the various types of mutexes that KallistiOS
    supports.

    @{
*/
#define MUTEX_TYPE_NORMAL       0   /**< \brief Normal mutex type */
#define MUTEX_TYPE_OLDNORMAL    1   /**< \brief Alias for MUTEX_TYPE_NORMAL */
#define MUTEX_TYPE_ERRORCHECK   2   /**< \brief Error-checking mutex type */
#define MUTEX_TYPE_RECURSIVE    3   /**< \brief Recursive mutex type */

/** \brief Default mutex type */
#define MUTEX_TYPE_DEFAULT      MUTEX_TYPE_NORMAL
/** @} */

/** \brief  Initializer for a transient mutex. */
#define MUTEX_INITIALIZER               { MUTEX_TYPE_NORMAL, NULL, 0 }

/** \brief  Initializer for a transient error-checking mutex. */
#define ERRORCHECK_MUTEX_INITIALIZER    { MUTEX_TYPE_ERRORCHECK, NULL, 0 }

/** \brief  Initializer for a transient recursive mutex. */
#define RECURSIVE_MUTEX_INITIALIZER     { MUTEX_TYPE_RECURSIVE, NULL, 0 }

/** \brief  Initialize a new mutex.

    This function initializes a new mutex for use.

    \param  m               The mutex to initialize
    \param  mtype           The type of the mutex to initialize it to

    \retval 0               On success
    \retval -1              On error, errno will be set as appropriate

    \par    Error Conditions:
    \em     EINVAL - an invalid type of mutex was specified

    \sa     mutex_types
*/
int mutex_init(mutex_t *m, int mtype) __nonnull_all;

/** \brief  Destroy a mutex.

    This function destroys a mutex, releasing any memory that may have been
    allocated internally for it. It is your responsibility to make sure that all
    threads waiting on the mutex are taken care of before destroying the mutex.

    This function can be called on statically initialized as well as dynamically
    initialized mutexes.

    \param m                The mutex to destroy

    \retval 0               On success
    \retval -1              On error, errno will be set as appropriate

    \par    Error Conditions:
    \em     EBUSY - the mutex is currently locked
*/
int mutex_destroy(mutex_t *m) __nonnull_all;

/** \brief  Lock a mutex.

    This function will lock a mutex, if it is not already locked by another
    thread. If it is locked by another thread already, this function will block
    until the mutex has been acquired for the calling thread.

    The semantics of this function depend on the type of mutex that is used.

    \param  m               The mutex to acquire
    \retval 0               On success
    \retval -1              On error, sets errno as appropriate

    \par    Error Conditions:
    \em     EPERM - called inside an interrupt \n
    \em     EINVAL - the mutex has not been initialized properly \n
    \em     EAGAIN - lock has been acquired too many times (recursive) \n
    \em     EDEADLK - would deadlock (error-checking)
*/
int mutex_lock(mutex_t *m) __nonnull_all;

/** \brief  Lock a mutex.

    This function will lock a mutex, if it is not already locked by another
    thread. If it is locked by another thread already, this function will block
    until the mutex has been acquired for the calling thread.
    This function can be called from within an interrupt context. In that case,
    if the mutex is already locked, an error will be returned.

    The semantics of this function depend on the type of mutex that is used.

    \param  m               The mutex to acquire
    \retval 0               On success
    \retval -1              On error, sets errno as appropriate

    \par    Error Conditions:
    \em     EINVAL - the mutex has not been initialized properly \n
    \em     EAGAIN - lock has been acquired too many times (recursive), or the
                     function was called inside an interrupt and the mutex was
                     already locked \n
    \em     EDEADLK - would deadlock (error-checking)
*/
int mutex_lock_irqsafe(mutex_t *m) __nonnull_all;

/** \brief  Lock a mutex (with a timeout).

    This function will attempt to lock a mutex. If the lock can be acquired
    immediately, the function will return immediately. If not, the function will
    block for up to the specified number of milliseconds to wait for the lock.
    If the lock cannot be acquired in this timeframe, this function will return
    an error.

    \param  m               The mutex to acquire
    \param  timeout         The number of milliseconds to wait for the lock
    \retval 0               On success
    \retval -1              On error, errno will be set as appropriate

    \par    Error Conditions:
    \em     EPERM - called inside an interrupt \n
    \em     EINVAL - the mutex has not been initialized properly \n
    \em     EINVAL - the timeout value was invalid (less than 0) \n
    \em     ETIMEDOUT - the timeout expired \n
    \em     EAGAIN - lock has been acquired too many times (recursive) \n
    \em     EDEADLK - would deadlock (error-checking)
*/
int mutex_lock_timed(mutex_t *m, int timeout) __nonnull_all;

/** \brief  Check if a mutex is locked.

    This function will check whether or not a mutex is currently locked. This is
    not a thread-safe way to determine if the mutex will be locked by the time
    you get around to doing it. If you wish to attempt to lock a mutex without
    blocking, look at mutex_trylock(), not this.

    \param  m               The mutex to check
    \retval 0               If the mutex is not currently locked
    \retval 1               If the mutex is currently locked
*/
int mutex_is_locked(const mutex_t *m) __nonnull_all;

/** \brief  Attempt to lock a mutex.

    This function will attempt to acquire the mutex for the calling thread,
    returning immediately whether or not it could be acquired. If the mutex
    cannot be acquired, an error will be returned.

    This function is safe to call inside an interrupt.

    \param  m               The mutex to attempt to acquire
    \retval 0               On successfully acquiring the mutex
    \retval -1              If the mutex cannot be acquired without blocking

    \par    Error Conditions:
    \em     EBUSY  - the mutex is already locked (mutex_lock() would block) \n
    \em     EINVAL - the mutex has not been initialized properly \n
    \em     EAGAIN - lock has been acquired too many times (recursive) \n
    \em     EDEADLK - would deadlock (error-checking)
*/
int mutex_trylock(mutex_t *m) __nonnull_all;

/** \brief  Unlock a mutex.

    This function will unlock a mutex, allowing other threads to acquire it.
    The semantics of this operation depend on the mutex type in use.

    \param  m               The mutex to unlock
    \retval 0               On success
    \retval -1              On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EPERM - the current thread does not own the mutex (error-checking or
                    recursive)
*/
int mutex_unlock(mutex_t *m) __nonnull_all;

/** \brief  Unlock a mutex under another thread's authority.

    This function allows an IRQ handler to unlock a mutex that was locked by a
    normal kernel thread. This function is only for use in IRQ handlers, so it
    will generally not be of much use outside of the kernel itself.

    \param  m               The mutex to unlock
    \param  thd             The thread owning the mutex
    \retval 0               On success
    \retval -1              On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EPERM - the specified thread does not own the mutex \n
    \em     EACCES - called outside an IRQ handler
*/
int mutex_unlock_as_thread(mutex_t *m, kthread_t *thd) __nonnull_all;

/** \cond */
static inline void __mutex_scoped_cleanup(mutex_t **m) {
    if(*m)
        mutex_unlock(*m);
}

#define ___mutex_lock_scoped(m, l) \
    mutex_t *__scoped_mutex_##l __attribute__((cleanup(__mutex_scoped_cleanup))) = mutex_lock(m) ? NULL : (m)

#define __mutex_lock_scoped(m, l) ___mutex_lock_scoped(m, l)
/** \endcond */

/** \brief  Lock a mutex with scope management

    This macro will lock a mutex, similarly to mutex_lock, with the difference
    that the mutex will automatically be unlocked once the execution exits the
    functional block in which the macro was called.

    \param  m               The mutex to acquire
*/
#define mutex_lock_scoped(m) __mutex_lock_scoped((m), __LINE__)

__END_DECLS

#endif  /* __KOS_MUTEX_H */
