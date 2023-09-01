/** \file   sys/sched.h
    \brief  Basic sys/sched.h file for newlib.

    This file specifies a few things to make sure pthreads stuff compiles.
*/

#ifndef __SYS_SCHED_H
#define __SYS_SCHED_H

#include <sys/cdefs.h>
__BEGIN_DECLS

// These are copied from Newlib to make stuff compile as expected.

#define SCHED_OTHER    0    /**< \brief Other scheduling */
#define SCHED_FIFO     1    /**< \brief FIFO scheduling */
#define SCHED_RR       2    /**< \brief Round-robin scheduling */

/** \brief  Scheduling Parameters, P1003.1b-1993, p. 249.
    \note   Fields whose name begins with "ss_" added by P1003.4b/D8, p. 33.
    \headerfile sys/sched.h
*/
struct sched_param {
    int sched_priority;           /**< \brief Process execution scheduling priority */
};

__END_DECLS

#endif  /* __SYS_SCHED_H */
