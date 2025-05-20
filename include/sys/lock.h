/* KallistiOS ##version##

   lock_common.h
   Copyright (C) 2004 Megan Potter

   This file is patched into the dc-chain newlib's <sys/lock.h>, by newlib.mk 
   in \utils\dc-chain\scripts.
*/

/** \file
    \brief   KOS-specific patching for newlib's <sys/lock.h>

    This file contains an implementation of the KOS threading back-end
    that will be patched into newlib by the toolchain make scripts.

    \author Megan Potter
*/

#ifndef __SYS_LOCK_H__
#define __SYS_LOCK_H__

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \cond */

typedef struct {
    void* owner;
    int nest;
    volatile int lock;
} __newlib_recursive_lock_t;

#define __NEWLIB_RECURSIVE_LOCK_INIT { (void *)0, 0, 0 }

typedef volatile int __newlib_lock_t;
#define __NEWLIB_LOCK_INIT 0

typedef unsigned long int _COND_T;
typedef __newlib_lock_t _LOCK_T;
typedef __newlib_recursive_lock_t _LOCK_RECURSIVE_T;

#define __LOCK_INIT(class,lock) class _LOCK_T lock = __NEWLIB_LOCK_INIT;
#define __LOCK_INIT_RECURSIVE(class,lock) class _LOCK_RECURSIVE_T lock = __NEWLIB_RECURSIVE_LOCK_INIT;
#define __lock_init(lock) __newlib_lock_init(&(lock))
#define __lock_init_recursive(lock) __newlib_lock_init_recursive(&(lock))
#define __lock_close(lock) __newlib_lock_close(&(lock))
#define __lock_close_recursive(lock) __newlib_lock_close_recursive(&(lock))
#define __lock_acquire(lock) __newlib_lock_acquire(&(lock))
#define __lock_acquire_recursive(lock) __newlib_lock_acquire_recursive(&(lock))
#define __lock_try_acquire(lock) __newlib_lock_try_acquire(&(lock))
#define __lock_try_acquire_recursive(lock) __newlib_lock_try_acquire_recursive(&(lock))
#define __lock_release(lock) __newlib_lock_release(&(lock))
#define __lock_release_recursive(lock) __newlib_lock_release_recursive(&(lock))

void __newlib_lock_init(__newlib_lock_t*);
void __newlib_lock_close(__newlib_lock_t*);
void __newlib_lock_acquire(__newlib_lock_t*);
int __newlib_lock_try_acquire(__newlib_lock_t*);
void __newlib_lock_release(__newlib_lock_t*);

void __newlib_lock_init_recursive(__newlib_recursive_lock_t*);
void __newlib_lock_close_recursive(__newlib_recursive_lock_t*);
void __newlib_lock_acquire_recursive(__newlib_recursive_lock_t*);
int __newlib_lock_try_acquire_recursive(__newlib_recursive_lock_t*);
void __newlib_lock_release_recursive(__newlib_recursive_lock_t*);

/** \endcond */

__END_DECLS

#endif /* __SYS_LOCK_H__ */
