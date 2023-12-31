/* KallistiOS ##version##

   kos/barrier.h
   Copyright (C) 2023 Lawrence Sebald
*/

#ifndef __KOS_BARRIER_H
#define __KOS_BARRIER_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \file    kos/barrier.h
    \brief   Definitions thread barriers.
    \ingroup barriers 

    This file contains the public API for KOS's kthread barrier mechanism.

    \author Lawrence Sebald
*/

/** \defgroup barriers Barriers
    \brief    KOS barrier API for kernel threads
    \ingroup  kthreads

    Barriers are a type of synchronization method which halt execution
    for group of threads until a certain number of them have reached
    the barrier. 

    @{
*/

#define THD_BARRIER_SERIAL_THREAD   0x7fffffff

/** \cond */
#ifndef __KTHREAD_HAVE_BARRIER_TYPE
#define __KTHREAD_HAVE_BARRIER_TYPE 1
/** \endcond */

#define THD_BARRIER_SIZE            64

/** \brief Opaque structure representing a kthread barrier */
typedef union kos_thd_barrier {
    /** \cond Opaque structure */
    unsigned char __opaque[THD_BARRIER_SIZE];
    long int __align;
    /** \endcond */
} thd_barrier_t;

/** \cond */
#endif /* !__KTHREAD_HAVE_BARRIER_TYPE */
/** \endcond */

int thd_barrier_init(thd_barrier_t *__RESTRICT barrier,
                     void *__RESTRICT attr, unsigned count);
int thd_barrier_destroy(thd_barrier_t *barrier);

int thd_barrier_wait(thd_barrier_t *barrier);

/** @} */

__END_DECLS

#endif /* !__KOS_BARRIER_H */
