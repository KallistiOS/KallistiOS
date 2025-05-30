/* KallistiOS ##version##

   include/kos/sem.h
   Copyright (C) 2001, 2003 Megan Potter
   Copyright (C) 2012 Lawrence Sebald

*/

/** \file    kos/sem.h
    \brief   Semaphores.
    \ingroup kthreads

    This file defines semaphores. A semaphore is a synchronization primitive
    that allows a specified number of threads to be in its critical section at a
    single point of time. Another way to think of it is that you have a
    predetermined number of resources available, and the semaphore maintains the
    resources.

    \author Megan Potter
    \see    kos/mutex.h
*/

#ifndef __KOS_SEM_H
#define __KOS_SEM_H

#include <kos/cdefs.h>

__BEGIN_DECLS

/** \brief  Semaphore type.

    This structure defines a semaphore. There are no public members of this
    structure for you to actually do anything with in your code, so don't try.

    \headerfile kos/sem.h
*/
typedef struct semaphore {
    int initialized;    /**< \brief Are we initialized? */
    int count;          /**< \brief The semaphore count */
} semaphore_t;

/** \brief  Initializer for a transient semaphore.
    \param  value           The initial count of the semaphore. */
#define SEM_INITIALIZER(value) { 1, value }

/** \brief  Initialize a semaphore for use.

    This function initializes the semaphore passed in with the starting count
    value specified.

    \param  sm              The semaphore to initialize
    \param  count           The initial count of the semaphore
    \retval 0               On success
    \retval -1              On error, errno will be set as appropriate

    \par    Error Conditions:
    \em     EINVAL - the semaphore's value is invalid (less than 0)
*/
int sem_init(semaphore_t *sm, int count);

/** \brief  Destroy a semaphore.

    This function destroys a semaphore, leaving it uninitialized. If there
    are any threads currently waiting on the semaphore, they will be woken
    with an ENOTRECOVERABLE error.

    \param  sm              The semaphore to destroy
    \retval 0               On success (no error conditions currently defined)
*/
int sem_destroy(semaphore_t *sm) __nonnull_all;

/** \brief  Wait on a semaphore.

    This function will decrement the semaphore's count and return, if resources
    are available. Otherwise, the function will block until the resources become
    available.

    This function does not protect you against doing things that will cause a
    deadlock. This function is not safe to call in an interrupt. See
    sem_trywait() for a safe function to call in an interrupt.

    \param  sm              The semaphore to wait on
    \retval 0               On success
    \retval -1              On error, sets errno as appropriate

    \par    Error Conditions:
    \em     EPERM - called inside an interrupt \n
    \em     EINVAL - the semaphore was not initialized
*/
int sem_wait(semaphore_t *sm) __nonnull_all;

/** \brief  Wait on a semaphore (with a timeout).

    This function will decrement the semaphore's count and return, if resources
    are available. Otherwise, the function will block until the resources become
    available or the timeout expires.

    This function does not protect you against doing things that will cause a
    deadlock. This function is not safe to call in an interrupt. See
    sem_trywait() for a safe function to call in an interrupt.

    \param  sm              The semaphore to wait on
    \param  timeout         The maximum number of milliseconds to block (a value
                            of 0 here will block indefinitely)
    \retval 0               On success
    \retval -1              On error, sets errno as appropriate

    \par    Error Conditions:
    \em     EPERM - called inside an interrupt \n
    \em     EINVAL - the semaphore was not initialized \n
    \em     EINVAL - the timeout value was invalid (less than 0) \n
    \em     ETIMEDOUT - timed out while blocking
 */
int sem_wait_timed(semaphore_t *sm, int timeout) __nonnull_all;

/** \brief  "Wait" on a semaphore without blocking.

    This function will decrement the semaphore's count and return, if resources
    are available. Otherwise, it will return an error.

    This function does not protect you against doing things that will cause a
    deadlock. This function, unlike the other waiting functions is safe to call
    inside an interrupt.

    \param  sm              The semaphore to "wait" on
    \retval 0               On success
    \retval -1              On error, sets errno as appropriate

    \par    Error Conditions:
    \em     EWOULDBLOCK - a call to sem_wait() would block \n
    \em     EINVAL - the semaphore was not initialized
*/
int sem_trywait(semaphore_t *sm) __nonnull_all;

/** \brief  Signal a semaphore.

    This function will release resources associated with a semaphore, signalling
    a waiting thread to continue on, if any are waiting. It is your
    responsibility to make sure you only release resources you have.

    \param  sm              The semaphore to signal
    \retval 0               On success
    \retval -1              On error, sets errno as appropriate

    \par    Error Conditions:
    \em     EINVAL - the semaphore was not initialized
*/
int sem_signal(semaphore_t *sm) __nonnull_all;

/** \brief  Retrieve the number of available resources.

    This function will retrieve the count of available resources for a
    semaphore. This is not a thread-safe way to make sure resources will be
    available when you get around to waiting, so don't use it as such.

    \param  sm              The semaphore to check
    \return                 The count of the semaphore (the number of resources
                            currently available)
*/
int sem_count(const semaphore_t *sm) __nonnull_all;

__END_DECLS

#endif  /* __KOS_SEM_H */
