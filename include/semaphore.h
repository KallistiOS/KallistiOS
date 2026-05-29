/* KallistiOS ##version##

   include/semaphore.h
   Copyright (C) 2026 Falco Girgis
*/

/*
    This file is an extremely lightweight wrapper which simply "extends" KOS's
    existing <kos/sem.h> kernel semaphores, which are already partially POSIX-
    compliant, and implements the rest of the missing POSIX functionality as a
    light-weight inline function wrapper API around them.
*/

#ifndef __SEMAPHORE_H
#define __SEMAPHORE_H

#include <kos/cdefs.h>
#include <kos/sem.h>
#include <errno.h>

__BEGIN_DECLS

/* POSIX semaphores are type-compatible with kernel semaphores. */
typedef semaphore_t sem_t; 

/* Selector macro, which implements function overload resolution for 2 and 3
   argument versions of the same function. Uses a "sliding argument" trick so
   that NAME is selected based on the number of arguments it gets passed.
*/
#define SEM_INIT_SELECTOR(_1, _2, _3, NAME, ...) NAME

/* Due to the fact that KOS's <kos/sem.h> is technically stomping on the POSIX
   symbol, sem_init(), by providing its own two argument version, we must
   implement some crazy macro shenanigans to allow both KOS's 2 argument
   version of sem_init() and POSIX's 3 argument version of sem_init() to
   coexist without causing errors on incompatible function declarations or
   duplicate symbols.
   
   What we do here, is use a macro named "sem_init()" to "hide" the KOS
   declaration with the same name, forwarding the arguments on as __VA_ARGS__
   into the selector macro, which will return either the identifier
   "sem_init_posix," in the case when 3 arguments are provided, or
   "(sem_init)," in the case when 2 arguments are provided.

   Finally, we simply invoke the return expression of SEM_INIT_SELECTOR() as
   if it were a function name (because it is). In the case of (sem_init)(),
   the parentheses around the identifier name allow us to escape from macro
   expansion, avoided infinite macro recursion and unhiding KOS's version of
   sem_init(), which we will call into.
*/
#define sem_init(...) \
    SEM_INIT_SELECTOR(__VA_ARGS__, sem_init_posix, (sem_init))(__VA_ARGS__)

/* 3-argument POSIX-compliant implementation of sem_init(). */
static inline int sem_init_posix(sem_t *sem, int shared, unsigned int value) {
    if(shared) {  /* We don't support shared semaphores. */
        errno = ENOSYS;
        return -1;
    } 
    else
        return sem_init(sem, value);
}

/* Forward POSIX sem_close() directly on to KOS's sem_destroy(). */
static inline int sem_close(sem_t* sem) {
    return sem_destroy(sem);
}

/* Implement POSIX's sem_getvalue() from KOS's sem_count(). */
static inline int sem_getvalue(sem_t *sem, int *value) {
    *value = sem_count(sem);
    return 0;
}

/* Forward POSIX's sem_post() on to KOS's sem_signal(). */
static inline int sem_post(sem_t *sem) {
    return sem_signal(sem);
}

__END_DECLS

#endif
