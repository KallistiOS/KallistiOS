/* KallistiOS ##version##

   newlib_env_lock.c
   Copyright (C)2004 Megan Potter

*/

// These calls can be nested.
#include <sys/reent.h>
#include <sys/lock.h>

static __newlib_recursive_lock_t lock = __NEWLIB_RECURSIVE_LOCK_INIT;

void __env_lock(struct _reent * r) {
    (void)r;
    __newlib_lock_acquire_recursive(&lock);
}

void __env_unlock(struct _reent * r) {
    (void)r;
    __newlib_lock_release_recursive(&lock);
}


