/* KallistiOS ##version##

   include/kos/cond.h
   Copyright (C) 2001, 2003 Megan Potter

*/

/** \file   kos/cond.h
    \brief  Condition variables.
    \ingroup kthreads

    This file contains the definition of a Condition Variable. Condition
    Variables (or condvars for short) are used with a mutex to act as a lock and
    checkpoint pair for threads.

    Basically, things work as follows (for the thread doing work):
    \li The associated mutex is locked.
    \li A predicate is checked to see if it is safe to do something.
    \li If it is not safe, you call cond_wait(), which releases the mutex.
    \li When cond_wait() returns, the mutex is reacquired, and work can go on.
    \li Update any predicates so that we know that the work is done, and unlock
        the mutex.

    Meanwhile, the thread updating the condition works as follows:
    \li Lock the mutex associated with the condvar.
    \li Produce work to be done.
    \li Call cond_signal() (with the associated mutex still locked), so that any
        threads waiting on the condvar will know they can continue on when the
        mutex is released, also update any predicates that say whether work can
        be done.
    \li Unlock the mutex so that worker threads can acquire the mutex and do
        whatever work needs to be done.

    Condition variables can be quite useful when used properly, and provide a
    fairly easy way to wait for work to be ready to be done.

    Condition variables should never be used with mutexes that are of the type
    MUTEX_TYPE_RECURSIVE. The lock will only be released once by the wait
    function, and thus you will end up deadlocking if you use a recursive mutex
    that has been locked more than once.

    \author Megan Potter
*/

#ifndef __KOS_COND_H
#define __KOS_COND_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <kos/thread.h>
#include <kos/mutex.h>

/** \brief  Condition variable.

    There are no public members of this structure for you to actually do
    anything with in your code, so don't try.

    \headerfile kos/cond.h
*/
typedef struct condvar {
    int dummy;
} condvar_t;

/** \brief  Initializer for a transient condvar. */
#define COND_INITIALIZER    { 0 }

/** \brief  Initialize a condition variable.

    This function initializes a new condition variable for use.

    \param  cv              The condition variable to initialize
    \retval 0               On success (no error conditions currently defined)
*/
int cond_init(condvar_t *cv);

/** \brief  Free a condition variable.

    This function destroys a condition variable (but not the mutex
    that is associated with it). This will also wake all threads waiting
    on the condition.

    \retval 0               On success (no error conditions currently defined)
*/
int cond_destroy(condvar_t *cv);

/** \brief  Wait on a condition variable.

    This function will wait on the condition variable, unlocking the mutex and
    putting the calling thread to sleep as one atomic operation. The wait in
    this function has no timeout, and will sleep forever if the condition is not
    signalled.

    The mutex will be locked and owned by the calling thread on return,
    regardless of whether it is a successful or error return.

    \param  cv              The condition to wait on
    \param  m               The associated mutex
    \retval 0               On success
    \retval -1              On error, sets errno as appropriate

    \par    Error Conditions:
    \em     EPERM - called inside an interrupt \n
    \em     EINVAL - the condvar was not initialized \n
    \em     EINVAL - the mutex is not initialized or not locked \n
    \em     ENOTRECOVERABLE - the condvar was destroyed while waiting
*/
int cond_wait(condvar_t *cv, mutex_t * m);

/** \brief  Wait on a condition variable with a timeout.

    This function will wait on the condition variable, unlocking the mutex and
    putting the calling thread to sleep as one atomic operation. If the timeout
    elapses before the condition is signalled, this function will return error.
    If a timeout of 0 is given, the call is equivalent to cond_wait() (there is
    no timeout).

    The mutex will be locked and owned by the calling thread on return,
    regardless of whether it is a successful or error return.

    \param  cv              The condition to wait on
    \param  m               The associated mutex
    \param  timeout         The number of milliseconds before timeout
    \retval 0               On success
    \retval -1              On error, sets errno as appropriate

    \par    Error Conditions:
    \em     EPERM - called inside an interrupt \n
    \em     ETIMEDOUT - timed out \n
    \em     EINVAL - the condvar was not initialized \n
    \em     EINVAL - the mutex is not initialized or not locked \n
    \em     ENOTRECOVERABLE - the condvar was destroyed while waiting
*/
int cond_wait_timed(condvar_t *cv, mutex_t * m, int timeout);

/** \brief  Signal a single thread waiting on the condition variable.

    This function will wake up a single thread that is waiting on the condition.
    The calling thread should be holding the associated mutex or recursive lock
    before calling this to guarantee sane behavior.

    \param  cv              The condition to signal
    \retval 0               On success
    \retval -1              On error, errno will be set as appropriate

    \par    Error Conditions:
    \em     EINVAL - the condvar was not initialized
*/
int cond_signal(condvar_t *cv);

/** \brief  Signal all threads waiting on the condition variable.

    This function will wake up all threads that are waiting on the condition.
    The calling thread should be holding the associated mutex or recursive lock
    before calling this to guarantee sane behavior.

    \param  cv              The condition to signal
    \retval 0               On success
    \retval -1              On error, errno will be set as appropriate

    \par    Error Conditions:
    \em     EINVAL - the condvar was not initialized
*/
int cond_broadcast(condvar_t *cv);

__END_DECLS

#endif  /* __KOS_COND_H */
