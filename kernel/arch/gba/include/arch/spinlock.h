/* KallistiOS 2.0.0

   arch/gba/include/spinlock.h
   (c)2001 Dan Potter

*/

#ifndef __ARCH_SPINLOCK_H
#define __ARCH_SPINLOCK_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/thread.h>

/* Defines processor specific spinlocks; if you include this file, you
   must first include kos/thread.h. */

/* Spinlock data type */
/* typedef int spinlock_t; */
typedef char spinlock_t;

/* Value initializer */
#define SPINLOCK_INITIALIZER 0

/* Initialize a spinlock */
#define spinlock_init(A) *(A) = SPINLOCK_INITIALIZER

/* Spin on a lock */
/* #define spinlock_lock(A) do { \
        if (*(A)) { \
            while (*(A)) \
                ; \
            *(A) = 1; \
        } \
    } while (0) */
#define spinlock_lock(A) do { } while (0)

/* Free a lock */
/* #define spinlock_unlock(A) do { \
        *(A) = 0; \
    } while (0) */
#define spinlock_unlock(A) do { } while (0)

#define spinlock_is_locked(A) ( *(A) != 0 )

__END_DECLS

#endif  /* __ARCH_SPINLOCK_H */

