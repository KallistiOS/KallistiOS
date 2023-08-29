/* KallistiOS ##version##

   pthread.h
   Copyright (C) 2023 Lawrence Sebald

*/

/** \file   pthread.h
    \brief  POSIX threading support.

    This file contains functions and declarations related to POSIX threading
    support. Please note that this support is not anywhere near actually POSIX-
    compliant, but it provides much of the functionality that is useful in
    porting code to KOS.

    It is not recommended to use this POSIX threading support in code designed
    specifically for KOS -- instead it is recommended to use the built-in
    threading support provided in-kernel.

    \author Lawrence Sebald
*/

#ifndef __PTHREAD_H
#define __PTHREAD_H

#include <sys/cdefs.h>
#include <sys/features.h>
#include <sys/_pthread_types.h>

#include <sched.h>
#include <time.h>

__BEGIN_DECLS

/* Process shared/private flag. Since we don't support multiple processes, these
   don't actually do anything different. */
#define PTHREAD_PROCESS_PRIVATE     0
#define PTHREAD_PROCESS_SHARED      1

/* Scope handling. We only support PTHREAD_SCOPE_SYSTEM (although, we don't
   actually support processes, so maybe they should be the same?) */
#define PTHREAD_SCOPE_PROCESS       0
#define PTHREAD_SCOPE_SYSTEM        1

#define PTHREAD_CANCEL_DISABLE      0
#define PTHREAD_CANCEL_ENABLE       1

#define PTHREAD_CANCEL_DEFERRED     0
#define PTHREAD_CANCEL_ASYNCHRONOUS 1

#define PTHREAD_CREATE_DETACHED     0
#define PTHREAD_CREATE_JOINABLE     1

#define PTHREAD_STACK_MIN           256
#define PTHREAD_STACK_MIN_ALIGNMENT 32

/* Threads */
extern int pthread_create(pthread_t *__RESTRICT thread,
                          const pthread_attr_t *__RESTRICT attr,
                          void *(*start_routine)(void *), void *__RESTRICT arg);
extern int pthread_detach(pthread_t thread);
extern int pthread_equal(pthread_t t1, pthread_t t2);
extern void pthread_exit(void *value_ptr);
extern int pthread_join(pthread_t thread, void **value_ptr);
extern pthread_t pthread_self(void);
extern int pthread_getname_np(pthread_t thread, char *buf, size_t buflen);
extern int pthread_setname_np(pthread_t thread, const char *buf); 

/* Thread attributes */
extern int pthread_attr_init(pthread_attr_t *attr);
extern int pthread_attr_destroy(pthread_attr_t *attr);

extern int pthread_attr_getdetachstate(const pthread_attr_t *attr,
                                       int *detachstate);
extern int pthread_attr_setdetatchstate(pthread_attr_t *attr, int detachstate);

extern int pthread_attr_getguardsize(const pthread_attr_t *__RESTRICT attr,
                                     size_t *__RESTRICT guardsize);
extern int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize);

extern int pthread_attr_getschedparam(const pthread_attr_t *__RESTRICT attr,
                                      struct sched_param *__RESTRICT param);
extern int pthread_attr_setschedparam(pthread_attr_t *__RESTRICT attr,
                                      const struct sched_param *__RESTRICT par);
extern int pthread_attr_getstack(const pthread_attr_t *__RESTRICT attr,
                                 void **__RESTRICT stackaddr,
                                 size_t *__RESTRICT stacksize);
extern int pthread_attr_setstack(pthread_attr_t *__RESTRICT attr,
                                 void *__RESTRICT stackaddr, size_t stacksize);
extern int pthread_attr_getstacksize(const pthread_attr_t *__RESTRICT attr,
                                     size_t *__RESTRICT stacksize);
extern int pthread_attr_setstacksize(pthread_attr_t *attr,
                                     size_t stacksize);
extern int pthread_attr_getscope(const pthread_attr_t *__RESTRICT attr,
                                 int *__RESTRICT contentionscope);
extern int pthread_attr_setscope(pthread_attr_t *attr, int contentionscope);
extern int pthread_attr_getname_np(const pthread_attr_t *__RESTRICT attr,
                                   char *__RESTRICT buf, size_t buflen);
extern int pthread_attr_setname_np(pthread_attr_t *__RESTRICT attr,
                                   const char *__RESTRICT name);

/* Thread cancellation (Not supported) */
extern int pthread_cancel(pthread_t thd);
extern void pthread_testcancel(void);
extern int pthread_setcancelstate(int state, int *oldstate);
extern int pthread_setcanceltype(int type, int *oldtype);

/* Condition variables */
extern int pthread_cond_init(pthread_cond_t *__RESTRICT cond,
                             const pthread_condattr_t *__RESTRICT attr);
extern int pthread_cond_destroy(pthread_cond_t *cond);

#define PTHREAD_COND_INITIALIZER    { .__data = { 0 } }

extern int pthread_cond_broadcast(pthread_cond_t *cond);
extern int pthread_cond_signal(pthread_cond_t *cond);
extern int pthread_cond_wait(pthread_cond_t *__RESTRICT cond,
                             pthread_mutex_t *__RESTRICT mutex);
extern int pthread_cond_timedwait(pthread_cond_t *__RESTRICT cond,
                                  pthread_mutex_t *__RESTRICT mutex,
                                  const struct timespec *__RESTRICT abstime);

extern int pthread_condattr_init(pthread_condattr_t *attr);
extern int pthread_condattr_destroy(pthread_condattr_t *attr);

extern int pthread_condattr_getclock(const pthread_condattr_t *__RESTRICT attr,
                                     clockid_t *__RESTRICT clock_id);
extern int pthread_condattr_setclock(pthread_condattr_t *attr,
                                     clockid_t clock_id);

/* Thread-specific data */
typedef int pthread_key_t;
extern int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
extern int pthread_key_delete(pthread_key_t key);
extern void *pthread_getspecific(pthread_key_t key);
extern int pthread_setspecific(pthread_key_t key, const void *value);

/* Mutexes */
extern int pthread_mutex_init(pthread_mutex_t *__RESTRICT mutex,
                              const pthread_mutexattr_t *__RESTRICT attr);
extern int pthread_mutex_destroy(pthread_mutex_t *mutex);

#define PTHREAD_MUTEX_INITIALIZER   { .__data = { 0 } }
#define PTHREAD_MUTEX_NORMAL        0
#define PTHREAD_MUTEX_DEFAULT       PTHREAD_MUTEX_NORMAL
#define PTHREAD_MUTEX_ERRORCHECK    2
#define PTHREAD_MUTEX_RECURSIVE     3

#define PTHREAD_MUTEX_ROBUST        0
#define PTHREAD_MUTEX_STALLED       1

extern int pthread_mutex_lock(pthread_mutex_t *mutex);
extern int pthread_mutex_trylock(pthread_mutex_t *mutex);
extern int pthread_mutex_timedlock(pthread_mutex_t *__RESTRICT mutex,
                                   const struct timespec *__RESTRICT abstime);
extern int pthread_mutex_unlock(pthread_mutex_t *mutex);
extern int pthread_mutex_consistent(pthread_mutex_t *mutex);

extern int pthread_mutexattr_init(pthread_mutexattr_t *attr);
extern int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);

extern int pthread_mutexattr_getrobust(const pthread_mutexattr_t *__RESTRICT at,
                                       int *__RESTRICT robust);
extern int pthread_mutexattr_setrobust(pthread_mutexattr_t *attr, int robust);

extern int pthread_mutexattr_gettype(const pthread_mutexattr_t *__RESTRICT attr,
                                     int *__RESTRICT type);
extern int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);

/* Dynamic package initialization */
typedef volatile int pthread_once_t;
#define PTHREAD_ONCE_INIT           0

extern int pthread_once(pthread_once_t *once_control,
                        void (*init_routine)(void));

/* Reader/writer locks */
extern int pthread_rwlock_init(pthread_rwlock_t *__RESTRICT rwlock,
                               const pthread_rwlockattr_t *__RESTRICT attr);
extern int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);

#define PTHREAD_RWLOCK_INITIALIZER  { .__data = { 0 } }

extern int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
extern int pthread_rwlock_timedrdlock(pthread_rwlock_t *__RESTRICT rwlock,
                                      const struct timespec *__RESTRICT abstm);
extern int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
extern int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
extern int pthread_rwlock_timedwrlock(pthread_rwlock_t *__RESTRICT rwlock,
                                      const struct timespec *__RESTRICT abstm);
extern int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
extern int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);

extern int pthread_rwlockattr_init(pthread_rwlockattr_t *attr);
extern int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr);

/* Spin locks */
typedef volatile int pthread_spinlock_t;

extern int pthread_spin_init(pthread_spinlock_t *lock, int pshared);
extern int pthread_spin_destroy(pthread_spinlock_t *lock);
extern int pthread_spin_lock(pthread_spinlock_t *lock);
extern int pthread_spin_trylock(pthread_spinlock_t *lock);
extern int pthread_spin_unlock(pthread_spinlock_t *lock);

/* Misc. */
extern int pthread_getconcurrency(void);
extern int pthread_setconcurrency(int new_level);
extern int pthread_atfork(void (*prepare)(void), void (*parent)(void),
                          void (*child)(void));

__END_DECLS

#endif /* !__PTHREAD_H */
