/* KallistiOS ##version##

   atomics.c
   Copyright (C) 2023 Falco Girgis
*/

/* This file provides the additional symbols required to provide
   support for C11 atomics with the "-matomic-model=soft-imask"
   build flag.
*/

#include <arch/arch.h>
#include <arch/cache.h>
#include <arch/irq.h>
#include <arch/spinlock.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Create a set of macros to codegen atomic symbols for primitive types.
   For these types, we simply disable interrupts then re-enable them
   around accesses to our atomics to ensure their atomicity.
*/
#define ATOMIC_LOAD_N_(type, n) \
    type \
    __atomic_load_##n(const volatile void *ptr, int model) { \
        (void)model; \
        irq_disable_scoped(); \
        return *(type *)ptr; \
    }

#define ATOMIC_STORE_N_(type, n) \
    void \
    __atomic_store_##n(volatile void *ptr, type val, int model) { \
        (void)model; \
        irq_disable_scoped(); \
        *(type *)ptr = val; \
    }

#define ATOMIC_EXCHANGE_N_(type, n) \
    type \
    __atomic_exchange_##n(volatile void* ptr, type val, int model) { \
        irq_disable_scoped(); \
        const type ret = *(type *)ptr; \
        (void)model; \
        *(type*)ptr = val; \
        return ret; \
    }

#define ATOMIC_COMPARE_EXCHANGE_N_(type, n) \
    bool \
    __atomic_compare_exchange_##n(volatile void *ptr, \
                                  void *expected, \
                                  type desired, \
                                  bool weak, \
                                  int success_memorder, \
                                  int failure_memorder) { \
        (void)weak; \
        (void)success_memorder; \
        (void)failure_memorder; \
        irq_disable_scoped(); \
        if(*(type *)ptr == *(type *)expected) { \
            *(type *)ptr = desired; \
            return true; \
        } else { \
            *(type *)expected = *(type *)ptr; \
            return false; \
        } \
    }

#define ATOMIC_FETCH_N_(type, n, opname, op) \
    type \
    __atomic_fetch_##opname##_##n(volatile void* ptr, \
                                  type val, \
                                  int memorder) { \
        irq_disable_scoped(); \
        type ret = *(type *)ptr; \
        (void)memorder; \
        *(type *)ptr op val; \
        return ret;  \
    }

#define ATOMIC_FETCH_NAND_N_(type, n) \
    type \
    __atomic_fetch_nand_##n(volatile void* ptr, \
                            type val, \
                            int memorder) { \
        irq_disable_scoped(); \
        type ret = *(type *)ptr; \
        (void)memorder; \
        *(type *)ptr = ~(*(type *)ptr & val); \
        return ret;  \
    }

/* GCC provides us with all primitive atomics except for 64-bit types. */
ATOMIC_LOAD_N_(unsigned long long, 8)
ATOMIC_STORE_N_(unsigned long long, 8)
ATOMIC_EXCHANGE_N_(unsigned long long, 8)
ATOMIC_COMPARE_EXCHANGE_N_(unsigned long long, 8)
ATOMIC_FETCH_N_(unsigned long long, 8, add, +=)
ATOMIC_FETCH_N_(unsigned long long, 8, sub, -=)
ATOMIC_FETCH_N_(unsigned long long, 8, and, &=)
ATOMIC_FETCH_N_(unsigned long long, 8, or, |=)
ATOMIC_FETCH_N_(unsigned long long, 8, xor, ^=)
ATOMIC_FETCH_NAND_N_(unsigned long long, 8)

/* Provide GCC with symbols and logic required to implement
   generically sized atomics. Rather than disabling an enabling
   interrupts around primitive assignments, we use spinlocks
   around memcpy() calls. */

/* Size of each memory region covered by an individual lock. */
#define GENERIC_LOCK_BLOCK_SIZE     (CPU_CACHE_BLOCK_SIZE * 4)

/* Locks have to be shared for each page with the MMU enabled,
   otherwise we can fail when aliasing an address range to multiple
   pages. */
#define GENERIC_LOCK_COUNT          (PAGESIZE / GENERIC_LOCK_BLOCK_SIZE)

/* Create a hash table mapping a region of memory to a lock. */
static spinlock_t locks[GENERIC_LOCK_COUNT] = {
    [0 ... (GENERIC_LOCK_COUNT - 1)] = SPINLOCK_INITIALIZER
};

/* Our hash function which maps an address to its corresponding lock index. */
inline static uintptr_t
address_to_spinlock(const volatile void *ptr) {
    return ((uintptr_t)ptr / GENERIC_LOCK_BLOCK_SIZE) % GENERIC_LOCK_COUNT;
}

void __atomic_load(size_t size,
                   const volatile void *ptr,
                   void *ret,
                   int memorder) {
    const uintptr_t lock = address_to_spinlock(ptr);

    (void)memorder;

    spinlock_lock_scoped(&locks[lock]);
    memcpy(ret, (const void *)ptr, size);
}

void __atomic_store(size_t size,
                    volatile void *ptr,
                    void *val,
                    int memorder) {
    const uintptr_t lock = address_to_spinlock(ptr);

    (void)memorder;

    spinlock_lock_scoped(&locks[lock]);
    memcpy((void *)ptr, val, size);
}

void __atomic_exchange(size_t size,
                       volatile void *ptr,
                       void *val,
                       void *ret,
                       int memorder) {
    const uintptr_t lock = address_to_spinlock(ptr);

    (void)memorder;

    spinlock_lock_scoped(&locks[lock]);
    memcpy(ret, (const void *)ptr, size);
    memcpy((void *)ptr, val, size);
}

bool __atomic_compare_exchange(size_t size,
                               volatile void* ptr,
                               void* expected,
                               void* desired,
                               int success_memorder,
                               int fail_memorder) {
    const uintptr_t lock = address_to_spinlock(ptr);

    (void)success_memorder;
    (void)fail_memorder;

    spinlock_lock_scoped(&locks[lock]);

    if(memcmp((const void *)ptr, expected, size) == 0) {
        memcpy((void *)ptr, desired, size);
        return true;
    }
    else {
        memcpy(expected, (const void *)ptr, size);
        return false;
    }
}

/* All atomics for builtin types are lock-free, while our
   generic atomics back-end utilizes spinlocks. */
bool __atomic_is_lock_free(size_t size, const volatile void *ptr) {
    (void)ptr;
    (void)size;
    return (size <= sizeof(long long)) ? true : false;
}
