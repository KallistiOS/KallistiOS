/* KallistiOS ##version##

   threads.h
   Copyright (C) 2014 Lawrence Sebald
*/

/** \file   threads.h
    \brief  C11 Threading API.
    \ingroup threading_c11

    This file contains the definitions needed for using C11 threads. The C11
    standard defines a number of threading-related primitives, which we wrap
    neatly around KOS' built-in threading support here.

    If you compile your code with a strict standard set (you use a -std= flag
    with GCC that doesn't start with gnu), you must use -std=c11 to use this
    functionality. If you don't pass a -std= flag to GCC, then you're probably
    fine.

    \author Lawrence Sebald
*/

#ifndef __THREADS_H
#define __THREADS_H

#if !defined(__STRICT_ANSI__) || (__STDC_VERSION__ >= 201112L)

#include <kos/cdefs.h>
#include <time.h>

/* Bring in all the threading-related stuff we'll need. */
#include <kos/thread.h>
#include <kos/once.h>
#include <kos/mutex.h>
#include <kos/cond.h>
#include <kos/tls.h>

__BEGIN_DECLS

/** \defgroup threading_c11     C11
    \brief                      C11 Threading APIs
    \ingroup                    threading
    
    @{
*/

/** \name Return values
    \brief C11 Thread function return values

    Most of the C11 thread-related functions that return a result code return
    one of these.

    @{
*/
#define thrd_success    0                   /**< \brief Success */
#define thrd_error      -1                  /**< \brief Uncategorized error */
#define thrd_timedout   -2                  /**< \brief Time out error */
#define thrd_busy       -3                  /**< \brief Resource busy */
#define thrd_nomem      -4                  /**< \brief Out of memory */
/** @} */

/** \brief  Object type backing call_once.

    This object type holds a flag that is used by the call_once function to call
    a function one time. It should always be initialized with the ONCE_FLAG_INIT
    macro.

    \headerfile threads.h
*/
typedef kthread_once_t once_flag;

/** \brief  Macro to initialize a once_flag object. */
#define ONCE_FLAG_INIT KTHREAD_ONCE_INIT

/** \brief  Call a function one time, no matter how many threads try.

    This function uses the once_flag object passed in to ensure that a given
    function is called exactly once, regardless of how many threads attempt to
    call through the once_flag.

    \param  flag            The once_flag to run against.
    \param  func            The function to call.
*/
extern void call_once(once_flag *flag, void (*func)(void));

/** \brief  C11 mutual exclusion lock type.

    This type holds an identifier for a mutual exclusion (mutex) lock to be used
    with C11 threading support.

    \headerfile threads.h
*/
typedef mutex_t mtx_t;

/** \name  Mutex types
    \brief C11 mutual exclusion lock types

    These are the possible types of mutex locks that C11 allows for. Note that
    mtx_plain or mtx_recursive can be ORed with mtx_timed as well.

    @{
*/
#define mtx_plain       (1 << 0)            /**< \brief Plain mutex */
#define mtx_recursive   (1 << 1)            /**< \brief Recursive mutex */
#define mtx_timed       (1 << 2)            /**< \brief Mutex supporting the
                                                 mtx_timedlock function. */
/** @} */

/** \brief  Deinitialize a mutex lock.

    This function deinitializes a mutex lock that was previously created with
    mtx_init().

    \param  mtx         The mutex to deinitialize.
*/
extern void mtx_destroy(mtx_t *mtx);

/** \brief  Initialize a mutex lock.

    This function initializes a mutex lock of the given type for later use to
    protect critical sections of code.

    \param  mtx             The mutex to initialize.
    \param  type            The type of mutex desired
    \retval thrd_success    On success.
    \retval thrd_error      If the request could not be honored.
*/
extern int mtx_init(mtx_t *mtx, int type);

/** \brief  Lock a mutex lock.

    This function locks the specified mutex, preventing any other threads from
    obtaining the same lock.

    This function will block until the lock can be obtained.

    \param  mtx             The mutex to lock.
    \retval thrd_success    On success.
    \retval thrd_error      If the request could not be honored.

    \note   Calling this function in an interrupt will result in an error being
            returned.
*/
extern int mtx_lock(mtx_t *mtx);

/** \brief  Lock a mutex lock with a timeout.

    This function locks the specified mutex, assuming that the lock can be
    obtained in the time period specified.

    This function will block until the lock can be obtained or the timeout
    expires.

    \param  mtx             The mutex to lock.
    \param  ts              The amount of time to wait before timing out.
    \retval thrd_success    On success.
    \retval thrd_error      If the request could not be honored for some other
                            reason than a timeout.
    \retval thrd_timedout   If the timeout specified passes without obtaining
                            the lock.

    \note   Calling this function in an interrupt will result in an error being
            returned.
    \note   Although timeouts are specified in seconds and nanoseconds, the
            timeout will be rounded up to the nearest millisecond.
*/
extern int mtx_timedlock(mtx_t *__RESTRICT mtx,
                         const struct timespec *__RESTRICT ts);

/** \brief  Attempt to acquire a mutex lock.

    This function attempts to acquire the specified mutex and will not block if
    it cannot be obtained.

    \param  mtx             The mutex to lock.
    \retval thrd_success    On success.
    \retval thrd_busy       If the lock is already locked by a thread.
    \retval thrd_error      If the request could not be honored for some other
                            reason.

    \note   This function is safe to call in an interrupt.
    \note   Always check the return value to ensure that the lock was obtained.
*/
extern int mtx_trylock(mtx_t *mtx);

/** \brief  Unlock a previously acquired lock.

    This function releases the specified mutex lock, allowing other threads to
    acquire it.

    \param  mtx             The mutex to unlock.
    \retval thrd_success    On success.
    \retval thrd_error      If the request cannot be honored.

    \note   Unlocking a mutex that was not previously locked by the calling
            thread results in undefined behavior.
*/
extern int mtx_unlock(mtx_t *mtx);

/** \brief  C11 condition variable type.

    This type holds an identifier for a condition variable object that is to be
    used with C11 threading support.

    \headerfile threads.h
*/
typedef condvar_t cnd_t;

/** \brief  Broadcast to all threads locked on a condition variable.

    This function wakes all threads that are blocked on the condition variable
    cond at the time of the call. If no threads are currently blocked on cond,
    this call does nothing.

    \param  cond            The condition variable to signal.
    \retval thrd_success    On success.
    \retval thrd_error      If the request cannot be honored.
*/
extern int cnd_broadcast(cnd_t *cond);

/** \brief  Deinitialize a condition variable.

    This function cleans up all resources associated with the given condition
    variable. You must ensure that no threads are currently blocked on the
    condition variable before calling this function.

    \param  cond            The condition variable to deinitialize.

    \note   Deinitializing a condition variable that is currently being waited
            on by threads results in undefined behavior.
*/
extern void cnd_destroy(cnd_t *cond);

/** \brief  Initialize a condition variable.

    This function initializes the specified condition variable for use.

    \param  cond            The condition variable to signal.
    \retval thrd_success    On success.
    \retval thrd_nomem      If memory cannot be allocated for the new condition
                            variable.
    \retval thrd_error      If the request cannot be honored for some other
                            reason.
*/
extern int cnd_init(cnd_t *cond);

/** \brief  Signal one thread locked on a condition variable.

    This function wakes one thread that is blocked on the condition variable
    cond at the time of the call. If no threads are currently blocked on cond,
    this call does nothing.

    \param  cond            The condition variable to signal.
    \retval thrd_success    On success.
    \retval thrd_error      If the request cannot be honored.
*/
extern int cnd_signal(cnd_t *cond);

/** \brief  Wait on a condition variable (with a timeout).

    This function puts the calling thread to sleep until either the condition
    variable is signaled or the timeout specified expires, whichever happens
    first. The specified mutex must be held by the calling thread when calling
    this function and will be held by the thread again when it is unblocked.

    \param  cond            The condition variable to wait on.
    \param  mtx             The mutex associated with the condition variable.
    \param  ts              The time to wait before timing out.
    \retval thrd_success    On success.
    \retval thrd_timedout   If the timeout was reached before the condition
                            variable was signaled.
    \retval thrd_error      If the request cannot be honored for some other
                            reason.

    \note   Calling this function in an interrupt will result in an error being
            returned.
    \note   Although timeouts are specified in seconds and nanoseconds, the
            timeout will be rounded up to the nearest millisecond.
*/
extern int cnd_timedwait(cnd_t *__RESTRICT cond, mtx_t *__RESTRICT mtx,
                         const struct timespec *__RESTRICT ts);

/** \brief  Wait on a condition variable.

    This function puts the calling thread to sleep until the condition variable
    is signaled. The specified mutex must be held by the calling thread when
    calling this function and will be held by the thread again when it is
    unblocked.

    \param  cond            The condition variable to wait on.
    \param  mtx             The mutex associated with the condition variable.
    \retval thrd_success    On success.
    \retval thrd_error      If the request cannot be honored.

    \note   Calling this function in an interrupt will result in an error being
            returned.
*/
extern int cnd_wait(cnd_t *cond, mtx_t *mtx);

/** \brief  C11 thread identifier type.

    This type holds an identifier for a C11 thread.

    \headerfile threads.h
*/
typedef kthread_t *thrd_t;

/** \brief  C11 thread start function type.

    This is a function pointer type representing a function used to begin a
    thread. The thread exits when the function returns or calls thrd_exit().

    \headerfile threads.h
*/
typedef int (*thrd_start_t)(void *);

/** \brief  Create and start a new thread.

    This function creates a new thread, calling the function specified. The
    thread is immediately added to the runnable queue of the scheduler and can
    start at any moment after that. The thread ends when either the function
    specified returns or when the thread calls thrd_exit().

    \param  thr             Storage for the thread identifier.
    \param  func            The function to call in the new thread.
    \param  arg             Argument to pass to the function called.
    \retval thrd_success    On success.
    \retval thrd_nomem      If memory cannot be allocated to satisfy the
                            request.
    \retval thrd_error      If the request cannot be honored for some other
                            reason.

    \note   All threads created are joinable threads by default. That means that
            in order to free all resources at thread termination, the thread
            must be joined with the thrd_join() function or detached at some
            point with thrd_detach().
*/
extern int thrd_create(thrd_t *thr, thrd_start_t func, void *arg);

/** \brief  Return the identifier of the currently running thread.

    \return                 The current thread's ID.
*/
extern thrd_t thrd_current(void);

/** \brief  Detach a running thread.

    This function detaches a thread, which informs the kernel that any resources
    associated with the thread should be freed immediately when it terminates.

    \param  thr             The thread to detach.
    \retval thrd_success    On success.
    \retval thrd_error      If the request cannot be honored.

    \note   Detaching an already detached thread has no effect.
    \note   Detaching a thread that has been joined with another thread results
            in undefined behavior.
*/
extern int thrd_detach(thrd_t thr);

/** \brief  Compare two threads for equality.

    This function checks the two two thread identifiers passed in to see if they
    refer to the same thread.

    \param  thr0            The first thread to compare.
    \param  thr1            The second thread to compare.
    \return                 0 if the threads are not equal, nonzero if the
                            threads are equal.
*/
extern int thrd_equal(thrd_t thr0, thrd_t thr1);

/** \brief  Terminate the current thread immediately.

    This function terminates the calling thread immediately, setting the return
    value of the thread to the value specified.

    \param  res             The return value of the thread.
    \note                   This function will not return.
*/
_Noreturn extern void thrd_exit(int res);

/** \brief  Join a running thread.

    This function joins the current thread with the specified thread, blocking
    until that thread has terminated.

    \param  thr             The thread to join with.
    \param  res             Pointer to storage for the result code of the other
                            thread. Set to NULL if you don't care about the
                            result value.
    \retval thrd_success    On success.
    \retval thrd_error      If the request cannot be honored.

    \note   Joining with a previously detached thread results in undefined
            behavior.
    \note   Joining with a thread that has already been joined to another thread
            results in undefined behavior.
    \note   Calling this function in an interrupt will result in an error being
            returned.
*/
extern int thrd_join(thrd_t thr, int *res);

/** \brief  Put the currently running thread to sleep.

    This function puts the currently running thread to sleep for the specified
    duration of time, returning any left over time (if interrupted by a signal,
    for instance) in the second parameter.

    \param  duration        The amount of time to sleep.
    \param  remaining       Any remaining time from the duration that the thread
                            did not sleep for.
    \return                 0 if the requested time elapsed, a negative value
                            otherwise.

    \note   Although the duration is expressed in seconds and nanoseconds, all
            sleeping is done in millisecond increments. The value specified will
            be rounded up if it is not an even number of milliseconds.
    \note   KOS does not support signals, so remaining will only ever have a
            value after the function if there is some sort of error.
    \note   Calling this function in an interrupt will result in an error being
            returned.
*/
extern int thrd_sleep(const struct timespec *duration,
                      struct timespec *remaining);

/** \brief  Yield the current thread's timeslice.

    This function immediately pauses the current thread's execution and switches
    to another thread in the ready queue (if there are any threads ready to
    execute).

    \note   Calling this function in an interrupt will not have any effect.
*/
extern void thrd_yield(void);

/** \brief  Maximum number of iterations over TSS destructors.

    This macro defines the maximum number of iterations that will be performed
    over the destructors for thread-specific storage objects when a thread
    terminates.
*/
#define TSS_DTOR_ITERATIONS     1

/** \brief  C11 thread-specific storage type.

    This type holds a thread-specific storage identifier, which allows a value
    to be associated with it for each and every thread running.

    \headerfile threads.h
*/
typedef kthread_key_t tss_t;

/** \brief  C11 thread-specific storage destructor type.

    This is a function pointer type which describes a destructor for a
    thread-specific storage object.

    \headerfile threads.h
*/
typedef void (*tss_dtor_t)(void *);

/** \brief  Create a thread-specific storage pointer.

    This function creates a thread-specific storage pointer and associates the
    destructor function supplied with it. After creating the pointer, each
    thread may associate a piece of data with the key.

    \param  key             The key to initialize.
    \param  dtor            The destructor to associate with the key.
    \retval thrd_success    On success.
    \retval thrd_error      On failure.
*/
extern int tss_create(tss_t *key, tss_dtor_t dtor);

/** \brief  Free resources associated with a thread-specific storage key.

    This function releases any resources used by the thread-specific storage
    key specified. Note that this DOES NOT call any destructors.

    \param  key             The key to deinitialize.
*/
extern void tss_delete(tss_t key);

/** \brief  Retrieve the value associated with a thread-specific storage key.

    This function retrieves the value associated with the specified
    thread-specific storage key and returns it to the caller. If no value has
    been set in the current thread, NULL is returned.

    \param  key             The key to look up the value associated with.
    \return                 The value associated with the key.
*/
extern void *tss_get(tss_t key);

/** \brief  Set the value associated with a thread-specific storage key.

    This function sets the value to be associated with the specified
    thread-specific storage key, overwriting any previous keys. Note that this
    DOES NOT call any destructors.

    \param  key             The key to set the value for.
    \param  val             The value to set.
    \retval thrd_success    On success.
    \retval thrd_error      If the request cannot be honored.
*/
extern int tss_set(tss_t key, void *val);

/** @} */

__END_DECLS

#endif /* !defined(__STRICT_ANSI__) || (__STDC_VERSION__ >= 201112L) */

#endif /* !__THREADS_H */
