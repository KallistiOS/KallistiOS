/** \file   sys/_pthread.h
    \brief  Basic sys/_pthread.h file for newlib.

    This file specifies a few things to make sure pthreads stuff compiles.
*/

#ifndef __SYS__PTHREAD_H
#define __SYS__PTHREAD_H

/* Make sure pthreads compile ok. */
/** \brief  POSIX threads supported (sorta) */
#define _POSIX_THREADS

/** \brief  POSIX timeouts supported (sorta) */
#define _POSIX_TIMEOUTS

#endif  /* __SYS__PTHREAD_H */
