/* KallistiOS ##version##

   kos/barrier.h
   Copyright (C) 2023 Lawrence Sebald
*/

#ifndef __KOS_BARRIER_H
#define __KOS_BARRIER_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \file   kos/barrier.h
    \brief  Definitions thread barriers.

    TODO: Write barrier description.

    \author Lawrence Sebald
*/

#define THD_BARRIER_SERIAL_THREAD   0x7fffffff;

#ifndef __KTHREAD_HAVE_BARRIER_TYPE
#define __KTHREAD_HAVE_BARRIER_TYPE 1
#define THD_BARRIER_SIZE            64

typedef union kos_thd_barrier {
    unsigned char __opaque[THD_BARRIER_SIZE];
    long int __align;
} thd_barrier_t;

#endif /* !__KTHREAD_HAVE_BARRIER_TYPE */

int thd_barrier_init(thd_barrier_t *__RESTRICT barrier,
                     void *__RESTRICT attr, unsigned count);
int thd_barrier_destroy(thd_barrier_t *barrier);

int thd_barrier_wait(thd_barrier_t *barrier);

__END_DECLS

#endif /* !__KOS_BARRIER_H */
