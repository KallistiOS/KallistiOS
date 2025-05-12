/* KallistiOS ##version##

   sys/_pthreadtypes.h
   Copyright (C) 2023, 2024 Lawrence Sebald

*/

#ifndef __SYS_PTHREADTYPES_H
#define __SYS_PTHREADTYPES_H

typedef unsigned long int pthread_t;

typedef struct pthread_mutexattr_t {
    int mtype;
    int robust;
} pthread_mutexattr_t;

typedef struct pthread_rwlockattr_t {
    /* Empty */
    char _unused;
} pthread_rwlockattr_t;

#ifndef __PTHREAD_HAVE_CONDATTR_TYPE
#define __PTHREAD_HAVE_CONDATTR_TYPE   1
#define __PTHREAD_CONDATTR_SIZE        16

typedef union pthread_condattr_t {
    unsigned char __data[__PTHREAD_CONDATTR_SIZE];
    long int __align;
} pthread_condattr_t;

#undef __PTHREAD_CONDATTR_SIZE
#endif /* !__PTHREAD_HAVE_CONDATTR_TYPE */

typedef struct pthread_barrierattr_t {
    /* Empty */
    char _unused;
} pthread_barrierattr_t;

/* The following types have no public elements. Their implementation is hidden
   from the public header. */

#ifndef __PTHREAD_HAVE_ATTR_TYPE
#define __PTHREAD_HAVE_ATTR_TYPE    1
#define __PTHREAD_ATTR_SIZE         32

typedef union pthread_attr_t {
    unsigned char __data[__PTHREAD_ATTR_SIZE];
    long int __align;
} pthread_attr_t;

#undef __PTHREAD_ATTR_SIZE
#endif /* !__PTHREAD_HAVE_ATTR_TYPE */

#ifndef __PTHREAD_HAVE_MUTEX_TYPE
#define __PTHREAD_HAVE_MUTEX_TYPE   1
#define __PTHREAD_MUTEX_SIZE        32

typedef union pthread_mutex_t {
    unsigned char __data[__PTHREAD_MUTEX_SIZE];
    long int __align;
} pthread_mutex_t;

#undef __PTHREAD_MUTEX_SIZE
#endif /* !__pthread_have_mutex_type */

#ifndef __PTHREAD_HAVE_COND_TYPE
#define __PTHREAD_HAVE_COND_TYPE    1
#define __PTHREAD_COND_SIZE         16

typedef union pthread_cond_t {
    unsigned char __data[__PTHREAD_COND_SIZE];
    long int __align;
} pthread_cond_t;

#undef __PTHREAD_COND_SIZE
#endif /* !__PTHREAD_HAVE_COND_TYPE */

#ifndef __PTHREAD_HAVE_RWLOCK_TYPE
#define __PTHREAD_HAVE_RWLOCK_TYPE  1
#define __PTHREAD_RWLOCK_SIZE       32

typedef union pthread_rwlock_t {
    unsigned char __data[__PTHREAD_RWLOCK_SIZE];
    long int __align;
} pthread_rwlock_t;

#undef __PTHREAD_RWLOCK_SIZE
#endif /* !__PTHREAD_HAVE_RWLOCK_TYPE */

#ifndef __PTHREAD_HAVE_BARRIER_TYPE
#define __PTHREAD_HAVE_BARRIER_TYPE 1
#define __PTHREAD_BARRIER_SIZE      64

typedef union pthread_barrier_t {
    unsigned char __data[__PTHREAD_BARRIER_SIZE];
    long int __align;
} pthread_barrier_t;

#undef __PTHREAD_BARRIER_SIZE
#endif /* !__PTHREAD_HAVE_BARRIER_TYPE */

#endif /* !__SYS_PTHREADTYPES_H */
