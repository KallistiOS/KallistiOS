/* KallistiOS ##version##

   atomics.c
   Copyright (C) 2023 Falco Girgis
*/

#include <arch/arch.h>
#include <arch/cache.h>
#include <arch/irq.h>
#include <arch/spinlock.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define GENERIC_LOCK_BLOCK_SIZE     (CPU_CACHE_BLOCK_SIZE * 4)
#define GENERIC_LOCK_COUNT          (PAGESIZE / GENERIC_LOCK_BLOCK_SIZE)

#define ATOMIC_LOAD_N_(type, n) \
    type \
    __atomic_load_##n(const volatile void *ptr, int model) { \
        (void)model; \
        const int irq = irq_disable(); \
        const type ret = *(type *)ptr; \
        irq_restore(irq); \
        return ret; \
    }

#define ATOMIC_STORE_N_(type, n) \
    void \
    __atomic_store_##n(volatile void *ptr, type val, int model) { \
        (void)model; \
        const int irq = irq_disable(); \
        *(type *)ptr = val; \
        irq_restore(irq); \
    }
    
#define ATOMIC_EXCHANGE_N_(type, n) \
    type \
    __atomic_exchange_##n(volatile void* ptr, type val, int model) { \
        (void)model; \
        const int irq = irq_disable(); \
        const type ret = *(type *)ptr; \
        *(type*)ptr = val; \
        irq_restore(irq); \
        return ret; \
    }

#define ATOMIC_COMPARE_EXCHANGE_N_(type, n) \
    bool \
    __atomic_compare_exchange_##n(volatile void *ptr, \
                                  void *expected, \
                                  type desired, \
                                  bool weak, \
                                  int success_memorder, \
                                  int failure_memorder) \
    { \
        (void)weak; \
        (void)success_memorder; \
        (void)failure_memorder; \
        const int irq = irq_disable(); \
        bool retval; \
        if(*(type *)ptr == *(type *)expected) { \
            *(type *)ptr = desired; \
            retval = true; \
        } else { \
            *(type *)expected = *(type *)ptr; \
            retval = false; \
        } \
        irq_restore(irq); \
        return retval; \
    }

#define ATOMIC_FETCH_N_(type, n, opname, op) \
    type \
    __atomic_fetch_##opname##_##n(volatile void* ptr, \
                                  type val, \
                                  int memorder) { \
        (void)memorder; \
        const int irq = irq_disable(); \
        type ret = *(type *)ptr; \
        *(type *)ptr op val; \
        irq_restore(irq); \
        return ret;  \
    }

#define ATOMIC_FETCH_NAND_N_(type, n) \
    type \
    __atomic_fetch_nand_##n(volatile void* ptr, \
                            type val, \
                            int memorder) { \
        (void)memorder; \
        const int irq = irq_disable(); \
        type ret = *(type *)ptr; \
        *(type *)ptr = ~(*(type *)ptr & val); \
        irq_restore(irq); \
        return ret;  \
    }

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

static spinlock_t locks[GENERIC_LOCK_COUNT] = {
    [0 ... (GENERIC_LOCK_COUNT - 1)] = SPINLOCK_INITIALIZER
};

inline static uintptr_t address_to_spinlock(const volatile void *ptr) {
    return ((uintptr_t)ptr / GENERIC_LOCK_BLOCK_SIZE) 
                % GENERIC_LOCK_COUNT;
}

void __atomic_load(size_t size, 
                   const volatile void *ptr, 
                   void *ret, 
                   int memorder)
{
    (void)memorder;

    const uintptr_t lock = address_to_spinlock(ptr);
    spinlock_lock(&locks[lock]);
    
    memcpy(ret, (const void *)ptr, size);
    
    spinlock_unlock(&locks[lock]);
}

void __atomic_store(size_t size, 
                    volatile void *ptr, 
                    void *val, 
                    int memorder) 
{
    (void)memorder;
    
    const uintptr_t lock = address_to_spinlock(ptr);
    spinlock_lock(&locks[lock]);
    
    memcpy((void *)ptr, val, size);
    
    spinlock_unlock(&locks[lock]);
}

void __atomic_exchange(size_t size,
                       volatile void *ptr,
                       void *val,
                       void *ret,
                       int memorder)
{
    (void)memorder;

    const uintptr_t lock = address_to_spinlock(ptr);
    spinlock_lock(&locks[lock]);
    
    memcpy(ret, (const void *)ptr, size);
    memcpy((void *)ptr, val, size);
    
    spinlock_unlock(&locks[lock]);                       
}

bool __atomic_compare_exchange(size_t size,
                               volatile void* ptr,
                               void* expected,
                               void* desired,
                               int success_memorder,
                               int fail_memorder)
{
    (void)success_memorder;
    (void)fail_memorder;

    bool retval;
    const uintptr_t lock = address_to_spinlock(ptr);
    
    spinlock_lock(&locks[lock]);
    
    if(memcmp((const void *)ptr, expected, size) == 0) {
        memcpy((void *)ptr, desired, size);
        retval = true;
    } else {
        memcpy(expected, (const void *)ptr, size);
        retval = false;
    }
    
    spinlock_unlock(&locks[lock]); 

    return retval;
}

bool __atomic_is_lock_free(size_t size, void *ptr) {
    (void)ptr;
    (void) size;
    return true;
}
