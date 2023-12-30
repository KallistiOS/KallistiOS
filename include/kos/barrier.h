/* KallistiOS ##version##

   kos/barrier.h
   Copyright (C) 2023 Lawrence Sebald
*/

#ifndef __KOS_BARRIER_H
#define __KOS_BARRIER_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \file       kos/barrier.h
    \brief      Thread barriers.
    \ingroup    kthreads

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

#define THD_BARRIER_SERIAL_THREAD   0x7fffffff

#ifndef __KTHREAD_HAVE_BARRIER_TYPE

/** \cond */
#define __KTHREAD_HAVE_BARRIER_TYPE 1
/** \endcond */

/** \brief  Size of a thread barrier, in bytes. */
#define THD_BARRIER_SIZE            64

typedef union kos_thd_barrier {
    unsigned char __opaque[THD_BARRIER_SIZE];
    long int __align;
} thd_barrier_t;

#endif /* !__KTHREAD_HAVE_BARRIER_TYPE */

int thd_barrier_init(thd_barrier_t *__RESTRICT barrier,
                     const void *__RESTRICT attr, unsigned count);
int thd_barrier_destroy(thd_barrier_t *barrier);

int thd_barrier_wait(thd_barrier_t *barrier);

__END_DECLS

#endif /* !__KOS_BARRIER_H */
