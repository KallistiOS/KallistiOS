/* KallistiOS ##version##

   arch/dreamcast/include/arch/spinlock.h
   Copyright (C) 2001 Megan Potter

*/

#ifndef __KOS_SPINLOCK_H
#pragma GCC warning "The `<arch/spinlock.h>` header has been moved to `<kos/spinlock.h>`."
#include <kos/spinlock.h>
#endif

#ifndef __ARCH_SPINLOCK_H
#define __ARCH_SPINLOCK_H

/* Defines processor specific spinlock implementation */

#include <stdbool.h>

/* Use a test-and-set to attempt to acquire a lock atomically.
   In one instruction, tas.b writes 0x80 to the spinlock and
   sets T flag to 1 if if the previous value was zero, or sets
   0 if the previous value was non-zero. Therefore, this function
   returns true if we've successfully locked the spinlock, or
   false if the spinlock was already taken.
*/

static inline bool arch_spinlock_trylock(spinlock_t *lock) {
    bool locked = false;

    __asm__ __volatile__("tas.b @%2\n\t"
                         "movt %0\n\t"
                         : "=r"(locked), "=m"(*lock)
                         : "r"(lock)
                         : "t");

    return locked;
}

#endif  /* __ARCH_SPINLOCK_H */
