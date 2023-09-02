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
#include <sys/_pthreadtypes.h>

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
int pthread_create(pthread_t *__RESTRICT thread,
                   const pthread_attr_t *__RESTRICT attr,
                   void *(*start_routine)(void *), void *__RESTRICT arg);
int pthread_detach(pthread_t thread);
int pthread_equal(pthread_t t1, pthread_t t2);
void pthread_exit(void *value_ptr);
int pthread_join(pthread_t thread, void **value_ptr);
pthread_t pthread_self(void);
int pthread_getname_np(pthread_t thread, char *buf, size_t buflen);
int pthread_setname_np(pthread_t thread, const char *buf);

/* Thread attributes */
int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);

int pthread_attr_getdetachstate(const pthread_attr_t *attr,
                                int *detachstate);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);

int pthread_attr_getguardsize(const pthread_attr_t *__RESTRICT attr,
                              size_t *__RESTRICT guardsize);
int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize);

int pthread_attr_getschedparam(const pthread_attr_t *__RESTRICT attr,
                               struct sched_param *__RESTRICT param);
int pthread_attr_setschedparam(pthread_attr_t *__RESTRICT attr,
                               const struct sched_param *__RESTRICT par);
int pthread_attr_getstack(const pthread_attr_t *__RESTRICT attr,
                          void **__RESTRICT stackaddr,
                          size_t *__RESTRICT stacksize);
int pthread_attr_setstack(pthread_attr_t *__RESTRICT attr,
                          void *__RESTRICT stackaddr, size_t stacksize);
int pthread_attr_getstacksize(const pthread_attr_t *__RESTRICT attr,
                              size_t *__RESTRICT stacksize);
int pthread_attr_setstacksize(pthread_attr_t *attr,
                              size_t stacksize);
int pthread_attr_getscope(const pthread_attr_t *__RESTRICT attr,
                          int *__RESTRICT contentionscope);
int pthread_attr_setscope(pthread_attr_t *attr, int contentionscope);
int pthread_attr_getname_np(const pthread_attr_t *__RESTRICT attr,
                            char *__RESTRICT buf, size_t buflen);
int pthread_attr_setname_np(pthread_attr_t *__RESTRICT attr,
                            const char *__RESTRICT name);

/* Thread cancellation (Not supported) */
int pthread_cancel(pthread_t thd);
void pthread_testcancel(void);
int pthread_setcancelstate(int state, int *oldstate);
int pthread_setcanceltype(int type, int *oldtype);

/* Condition variables */
int pthread_cond_init(pthread_cond_t *__RESTRICT cond,
                      const pthread_condattr_t *__RESTRICT attr);
int pthread_cond_destroy(pthread_cond_t *cond);

#define PTHREAD_COND_INITIALIZER    { .__data = { 0 } }

int pthread_cond_broadcast(pthread_cond_t *cond);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *__RESTRICT cond,
                      pthread_mutex_t *__RESTRICT mutex);
int pthread_cond_timedwait(pthread_cond_t *__RESTRICT cond,
                           pthread_mutex_t *__RESTRICT mutex,
                           const struct timespec *__RESTRICT abstime);

int pthread_condattr_init(pthread_condattr_t *attr);
int pthread_condattr_destroy(pthread_condattr_t *attr);

int pthread_condattr_getclock(const pthread_condattr_t *__RESTRICT attr,
                              clockid_t *__RESTRICT clock_id);
int pthread_condattr_setclock(pthread_condattr_t *attr,
                              clockid_t clock_id);

/* Thread-specific data */
typedef int pthread_key_t;
int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int pthread_key_delete(pthread_key_t key);
void *pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);

/* Mutexes */
int pthread_mutex_init(pthread_mutex_t *__RESTRICT mutex,
                       const pthread_mutexattr_t *__RESTRICT attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);

#define PTHREAD_MUTEX_INITIALIZER   { .__data = { 0 } }
#define PTHREAD_MUTEX_NORMAL        0
#define PTHREAD_MUTEX_DEFAULT       PTHREAD_MUTEX_NORMAL
#define PTHREAD_MUTEX_ERRORCHECK    2
#define PTHREAD_MUTEX_RECURSIVE     3

#define PTHREAD_MUTEX_ROBUST        0
#define PTHREAD_MUTEX_STALLED       1

int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_timedlock(pthread_mutex_t *__RESTRICT mutex,
                            const struct timespec *__RESTRICT abstime);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_consistent(pthread_mutex_t *mutex);

int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);

int pthread_mutexattr_getrobust(const pthread_mutexattr_t *__RESTRICT at,
                                int *__RESTRICT robust);
int pthread_mutexattr_setrobust(pthread_mutexattr_t *attr, int robust);

int pthread_mutexattr_gettype(const pthread_mutexattr_t *__RESTRICT attr,
                              int *__RESTRICT type);
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);

/* Dynamic package initialization */
typedef volatile int pthread_once_t;
#define PTHREAD_ONCE_INIT           0

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

/* Reader/writer locks */
int pthread_rwlock_init(pthread_rwlock_t *__RESTRICT rwlock,
                        const pthread_rwlockattr_t *__RESTRICT attr);
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);

#define PTHREAD_RWLOCK_INITIALIZER  { .__data = { 0 } }

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_timedrdlock(pthread_rwlock_t *__RESTRICT rwlock,
                               const struct timespec *__RESTRICT abstm);
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_timedwrlock(pthread_rwlock_t *__RESTRICT rwlock,
                               const struct timespec *__RESTRICT abstm);
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);

int pthread_rwlockattr_init(pthread_rwlockattr_t *attr);
int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr);

/* Spin locks */
typedef volatile int pthread_spinlock_t;

int pthread_spin_init(pthread_spinlock_t *lock, int pshared);
int pthread_spin_destroy(pthread_spinlock_t *lock);
int pthread_spin_lock(pthread_spinlock_t *lock);
int pthread_spin_trylock(pthread_spinlock_t *lock);
int pthread_spin_unlock(pthread_spinlock_t *lock);

/* Misc. */
int pthread_getconcurrency(void);
int pthread_setconcurrency(int new_level);
int pthread_atfork(void (*prepare)(void), void (*parent)(void),
                   void (*child)(void));

__END_DECLS

#endif /* !__PTHREAD_H */
