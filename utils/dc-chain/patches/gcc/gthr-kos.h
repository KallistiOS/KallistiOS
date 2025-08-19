/* Copyright (C) 2009, 2010, 2011, 2012, 2020 Lawrence Sebald */
/* This file is added to GCC during the patching stage of toolchain
   compilation. Any changes to this file will not take effect until
   the toolchain is recompiled. */

/* Threads compatibility routines for libgcc2 and libobjc.  */
/* Compile this one with gcc.  */
/* Copyright (C) 1997-2019 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */

#ifndef GCC_GTHR_KOS_H
#define GCC_GTHR_KOS_H

/* KallistiOS threads specific definitions. */

#define __GTHREADS 1
#define __GTHREADS_CXX0X 1
#define __GTHREAD_HAS_COND 1

#include <kos/thread.h>
#include <kos/tls.h>
#include <kos/mutex.h>
#include <kos/once.h>
#include <kos/cond.h>
#include <arch/irq.h>
#include <time.h>

/* 9.5.0 somehow requires this. Remove when no longer supported */
#include <pthread.h>

/* These should work just fine. */
typedef kthread_key_t __gthread_key_t;
typedef kthread_once_t __gthread_once_t;
typedef mutex_t __gthread_mutex_t;
typedef mutex_t __gthread_recursive_mutex_t;
typedef condvar_t __gthread_cond_t;
typedef kthread_t *__gthread_t;
typedef struct timespec __gthread_time_t;

#define __GTHREAD_ONCE_INIT KTHREAD_ONCE_INIT
#define __GTHREAD_MUTEX_INIT MUTEX_INITIALIZER
#define __GTHREAD_RECURSIVE_MUTEX_INIT RECURSIVE_MUTEX_INITIALIZER
#define __GTHREAD_COND_INIT COND_INITIALIZER
#define __GTHREAD_TIME_INIT {0,0}

#define __GTHREAD_MUTEX_INIT_FUNCTION __gthread_mutex_init_func

#ifdef _GTHREAD_USE_MUTEX_INIT_FUNC
# undef __GTHREAD_MUTEX_INIT
#endif
#ifdef _GTHREAD_USE_RECURSIVE_MUTEX_INIT_FUNC
# undef __GTHREAD_RECURSIVE_MUTEX_INIT
# undef __GTHREAD_RECURSIVE_MUTEX_INIT_FUNCTION
# define __GTHREAD_RECURSIVE_MUTEX_INIT_FUNCTION __gthread_recursive_mutex_init_func
#endif
#ifdef _GTHREAD_USE_COND_INIT_FUNC
# undef __GTHREAD_COND_INIT
# define __GTHREAD_COND_INIT_FUNCTION __gthread_cond_init_func
#endif


static inline int __gthread_active_p(void) {
    return 1;
}

#ifdef _LIBOBJC

/* This stuff only applies to Objective C. */

/* The config.h file in libobjc/ */
#include <config.h>

/* Key structure for maintaining thread specific storage */
static kthread_key_t _objc_thread_storage;

/* Backend initialization functions */

/* Initialize the threads subsystem. */
static inline int __gthread_objc_init_thread_system(void) {
    /* The only thing we have to do is to initialize the storage key. */
    return kthread_key_create(&_objc_thread_storage, NULL);
}

/* Close the threads subsystem. */
static inline int __gthread_objc_close_thread_system(void) {
    return kthread_key_delete(_objc_thread_storage);
}

/* Backend thread functions */

/* Create a new thread of execution. */
static inline objc_thread_t __gthread_objc_thread_detach(void (*func)(void *),
                                                         void *arg) {
    kthread_t *thd_hnd;

    /* Objective C threads are detached by default. */
    thd_hnd = thd_create(1, (void *)(void *)func, arg);
    return (objc_thread_t)thd_hnd;
}

/* Set the current thread's priority. */
static inline int __gthread_objc_thread_set_priority(int priority __attribute__((unused))) {
    /* XXXX */
    return -1;
}

/* Return the current thread's priority. */
static inline int __gthread_objc_thread_get_priority(void) {
    /* XXXX */
    return OBJC_THREAD_INTERACTIVE_PRIORITY;
}

/* Yield our process time to another thread. */
static inline void __gthread_objc_thread_yield(void) {
    thd_pass();
}

/* Terminate the current thread. */
static inline int __gthread_objc_thread_exit(void) {
    thd_exit(NULL);

    /* We shouldn't get here... */
    return -1;
}

/* Returns an integer value which uniquely describes a thread. */
static inline objc_thread_t __gthread_objc_thread_id(void) {
    return (objc_thread_t)thd_get_current();
}

/* Sets the thread's local storage pointer. */
static inline int __gthread_objc_thread_set_data(void *value) {
    return kthread_setspecific(_objc_thread_storage, value);
}

/* Returns the thread's local storage pointer. */
static inline void *__gthread_objc_thread_get_data(void) {
    return kthread_getspecific(_objc_thread_storage);
}

/* Backend mutex functions */

/* Allocate a mutex. */
static inline int __gthread_objc_mutex_allocate(objc_mutex_t mutex) {
    mutex->backend = objc_malloc(sizeof(mutex_t));

    if(mutex_init((mutex_t *)mutex->backend, MUTEX_TYPE_NORMAL)) {
        objc_free(mutex->backend);
        mutex->backend = NULL;
        return -1;
    }

    return 0;
}

/* Deallocate a mutex. */
static inline int __gthread_objc_mutex_deallocate(objc_mutex_t mutex) {
    mutex_t *m = (mutex_t *)mutex->backend;
    uint32 old;

    old = irq_disable();

    while(mutex_is_locked(m)) {
        mutex_unlock(m);
    }

    if(mutex_destroy(m))
        return -1;

    mutex->backend = NULL;
    irq_restore(old);

    objc_free(m);

    return 0;
}

/* Grab a lock on a mutex. */
static inline int __gthread_objc_mutex_lock(objc_mutex_t mutex) {
    return mutex_lock((mutex_t *)mutex->backend);
}

/* Try to grab a lock on a mutex. */
static inline int __gthread_objc_mutex_trylock(objc_mutex_t mutex) {
    return mutex_trylock((mutex_t *)mutex->backend);
}

/* Unlock the mutex. */
static inline int __gthread_objc_mutex_unlock(objc_mutex_t mutex) {
    return mutex_unlock((mutex_t *)mutex->backend);
}

/* Backend condition mutex functions */

/* Allocate a condition. */
static inline int __gthread_objc_condition_allocate(objc_condition_t cond) {
    cond->backend = objc_malloc(sizeof(condvar_t));

    if(cond_init((condvar_t *)cond->backend)) {
        objc_free(cond->backend);
        cond->backend = NULL;
        return -1;
    }

    return 0;
}

/* Deallocate a condition. */
static inline int __gthread_objc_condition_deallocate(objc_condition_t cond) {
    if(cond_destroy((condvar_t *)cond->backend))
        return -1;

    objc_free(cond->backend);
    cond->backend = NULL;
    return 0;
}

/* Wait on the condition. */
static inline int __gthread_objc_condition_wait(objc_condition_t cond,
                                                objc_mutex_t mutex) {
    return cond_wait((condvar_t *)cond->backend, (mutex_t *)mutex->backend);
}

/* Wake up all threads waiting on this condition. */
static inline int __gthread_objc_condition_broadcast(objc_condition_t cond) {
    return cond_broadcast((condvar_t *)cond->backend);
}

/* Wake up one thread waiting on this condition. */
static inline int __gthread_objc_condition_signal(objc_condition_t cond) {
    return cond_signal((condvar_t *)cond->backend);
}

#else /* _LIBOBJC */

static inline int __gthread_create(__gthread_t *thd, void *(*func)(void *),
                                   void *args) {
    /* Non Objective C threads are joinable by default. */
    *thd = thd_create(0, func, args);
    return (*thd == NULL);
}

static inline int __gthread_join(__gthread_t thd, void **value_ptr) {
    return thd_join(thd, value_ptr);
}

static inline int __gthread_detach(__gthread_t thd) {
    return thd_detach(thd);
}

static inline int __gthread_equal(__gthread_t t1, __gthread_t t2) {
    return t1 == t2;
}

static inline __gthread_t __gthread_self(void) {
    return thd_get_current();
}

static inline int __gthread_yield(void) {
    thd_pass();
    return 0;
}

static inline int __gthread_once(__gthread_once_t *__once,
                                 void (*__func)(void)) {
    return kthread_once(__once, __func);
}

static inline int __gthread_key_create(__gthread_key_t *__key,
                                       void (*__func)(void *)) {
    return kthread_key_create(__key, __func);
}

static int __gthread_key_delete(__gthread_key_t __key) {
    return kthread_key_delete(__key);
}

static inline void *__gthread_getspecific(__gthread_key_t __key) {
    return kthread_getspecific(__key);
}

static inline int __gthread_setspecific(__gthread_key_t __key,
                                        const void *__v) {
    return kthread_setspecific(__key, __v);
}

static inline void __gthread_mutex_init_func(__gthread_mutex_t *__mutex) {
    (void)mutex_init(__mutex, MUTEX_TYPE_NORMAL);
}

static inline int __gthread_mutex_destroy(__gthread_mutex_t *__mutex) {
    return mutex_destroy(__mutex);
}

static inline int __gthread_mutex_lock(__gthread_mutex_t *__mutex) {
    return mutex_lock(__mutex);
}

static inline int __gthread_mutex_trylock(__gthread_mutex_t *__mutex) {
    return mutex_trylock(__mutex);
}

#if _GTHREAD_USE_MUTEX_TIMEDLOCK
static inline int __gthread_mutex_timedlock(__gthread_mutex_t *m,
                                            const __gthread_time_t *timeout) {
    int t = (int)(timeout->tv_sec + (timeout->tv_nsec / 1000));
    return mutex_lock_timed(m, t);
}
#endif

static inline int __gthread_mutex_unlock(__gthread_mutex_t *__mutex) {
    return mutex_unlock(__mutex);
}

static inline void __gthread_recursive_mutex_init_func(__gthread_recursive_mutex_t *__mutex) {
    (void)mutex_init(__mutex, MUTEX_TYPE_RECURSIVE);
}

static inline int __gthread_recursive_mutex_lock(__gthread_recursive_mutex_t *__mutex) {
    return mutex_lock(__mutex);
}

static inline int __gthread_recursive_mutex_trylock(__gthread_recursive_mutex_t *__mutex) {
    return mutex_trylock(__mutex);
}

#if _GTHREAD_USE_MUTEX_TIMEDLOCK
static inline int __gthread_recursive_mutex_timedlock(__gthread_recursive_mutex_t *m,
                                                      const __gthread_time_t *timeout) {
    int t = (int)(timeout->tv_sec + (timeout->tv_nsec / 1000));
    return mutex_lock_timed(m, t);
}
#endif

static inline int __gthread_recursive_mutex_unlock(__gthread_recursive_mutex_t *__mutex) {
    return mutex_unlock(__mutex);
}

static inline int __gthread_recursive_mutex_destroy(__gthread_recursive_mutex_t *__mutex) {
    return mutex_destroy(__mutex);
}

#ifdef _GTHREAD_USE_COND_INIT_FUNC
static inline void __gthread_cond_init_func(__gthread_cond_t *__cond) {
    (void)cond_init(__cond);
}
#endif

static inline int __gthread_cond_broadcast(__gthread_cond_t *cond) {
    return cond_broadcast(cond);
}

static inline int __gthread_cond_signal(__gthread_cond_t *cond) {
    return cond_signal(cond);
}

static inline int __gthread_cond_wait(__gthread_cond_t *cond, __gthread_mutex_t *mutex) {
    return cond_wait(cond, mutex);
}

static inline int __gthread_cond_timedwait(__gthread_cond_t *cond,
                                           __gthread_mutex_t *mutex,
                                           const __gthread_time_t *timeout) {
    int t = (int)(timeout->tv_sec + (timeout->tv_nsec / 1000));
    return cond_wait_timed(cond, mutex, t);
}

static inline int __gthread_cond_wait_recursive(__gthread_cond_t *cond,
                                                __gthread_recursive_mutex_t *mutex) {
    return cond_wait(cond, mutex);
}

static inline int __gthread_cond_timedwait_recursive(__gthread_cond_t *cond,
                                                     __gthread_recursive_mutex_t *l,
                                                     const __gthread_time_t *timeout) {
    int t = (int)(timeout->tv_sec + (timeout->tv_nsec / 1000));
    return cond_wait_timed(cond, l, t);
}

static inline int __gthread_cond_destroy(__gthread_cond_t *cond) {
    return cond_destroy(cond);
}

#endif /* !_LIBOBJC */


#endif /* ! GCC_GTHR_KOS_H */
