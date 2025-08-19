/* KallistiOS ##version##

   kos/barrier.h
   Copyright (C) 2023 Lawrence Sebald
*/

#ifndef __KOS_BARRIER_H
#define __KOS_BARRIER_H

#include <kos/cdefs.h>
__BEGIN_DECLS

/** \file       kos/barrier.h
    \brief      Thread barriers.
    \ingroup    barriers

    Thread barriers are used to synchronize the progress of multiple threads. A
    barrier causes threads to wait until a specified number of threads have
    reached a certain execution point, ensuring a consistent state across
    different execution paths.

    This synchronization primitive is essential for scenarios in parallel
    programming where tasks executed by multiple threads must reach a certain
    point before any can proceed, ensuring data consistency and coordination
    among threads.

    \author     Lawrence Sebald
*/

/** \defgroup barriers Barriers
    \brief    KOS barrier API for kernel threads
    \ingroup  kthreads

    Barriers are a type of synchronization method which halt execution
    for group of threads until a certain number of them have reached
    the barrier. 

    @{
*/

/** \brief      Constant returned to one thread from pthread_barrier_wait().

    A single (unspecified) thread will be returned this value after successfully
    waiting on a barrier, with all other threads being returned a 0. This is
    useful for selecting one thread to perform any cleanup work associated with
    the barrier (or other serial work that must be performed).
*/
#define THD_BARRIER_SERIAL_THREAD   0x7fffffff

/** \cond */
#ifndef __KTHREAD_HAVE_BARRIER_TYPE

#define __KTHREAD_HAVE_BARRIER_TYPE 1
/** \endcond */

/** \brief  Size of a thread barrier, in bytes. */
#define THD_BARRIER_SIZE            64

/** \brief      Thread barrier type.

    Type used for implementing thread barriers. All members of this structure
    are private. Do not attempt to manipulate any data within any instances of
    this structure.

    \headerfile kos/barrier.h
*/
typedef union kos_thd_barrier {
    /** \cond Opaque structure */
    unsigned char __opaque[THD_BARRIER_SIZE];
    long int __align;
    /** \endcond */
} thd_barrier_t;

/** \cond */
#endif /* !__KTHREAD_HAVE_BARRIER_TYPE */
/** \endcond */

/** \brief      Initialize a thread barrier.

    This function initializes a thread barrier for use among the specified
    number of threads.

    \param  barrier         The barrier to initialize.
    \param  attr            Reserved for POSIX compatibility. Always pass NULL.
    \param  count           The number of threads the barrier will expect.

    \return 0 on success, non-zero error code on failure

    \par    Error Conditions:
    \em     EINVAL - NULL was passed for barrier \n
    \em     EINVAL - Non-NULL was passed for attr \n
    \em     EINVAL - count == 0 or count > UINT32_MAX
*/
int thd_barrier_init(thd_barrier_t *__RESTRICT barrier,
                     const void *__RESTRICT attr, unsigned count);

/** \brief      Destroy a thread barrier.

    This function destroys a thread barrier, releasing any resources associated
    with the barrier. Subsequent use of the barrier is results in undefined
    behavior unless it is later re-initialized with thd_barrier_init().

    \param  barrier         The barrier to destroy.

    \return 0 on success, non-zero error code on failure

    \par    Error Conditions:
    \em     EBUSY - A destroy operation is already in progress on barrier \n
    \em     EINVAL - NULL was passed for barrier \n
    \em     EINVAL - An already destroyed barrier was passed in \n
    \em     EPERM - Function was called in an interrupt context

    \note   This function may block if a wait operation is currently in progress
            on the specified barrier.
*/
int thd_barrier_destroy(thd_barrier_t *barrier);

/** \brief      Wait on a thread barrier.

    This function synchronizes the participating threads at the barrier
    specified. The calling thread will block until the required number of
    threads (specified by the barrier's count) have called this function to
    wait on the barrier.

    \param  barrier         The barrier to wait on.

    \return 0 or THD_BARRIER_SERIAL_THREAD on success, non-zero error
            code on failure

    \par    Error Conditions:
    \em     EINVAL - NULL was passed for barrier \n
    \em     EINVAL - barrier was destroyed via thd_barrier_destroy() \n
    \em     EINVAL - A call to thd_barrier_destroy() is in progress for
                     the specified barrier \n
    \em     EPERM - Function was called in an interrupt context
*/
int thd_barrier_wait(thd_barrier_t *barrier);

/** @} */

__END_DECLS

#endif /* !__KOS_BARRIER_H */
