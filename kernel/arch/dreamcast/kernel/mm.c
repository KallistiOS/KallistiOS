/* KallistiOS ##version##

   mm.c
   Copyright (C) 2000, 2001 Megan Potter
   Copyright (C) 2024 Falco Girgis
*/

/* Defines a simple UNIX-style memory pool system. */

#include <arch/types.h>
#include <arch/arch.h>
#include <arch/mm.h>
#include <arch/irq.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define MM_ALIGNMENT 4

/* End of program, exported from the linker script. */
extern unsigned long end;

/* Program Break Region Addresses */
static void *brk_start   = NULL; /* First Address */
static void *brk_current = NULL; /* Last Address at Size */
static void *brk_end     = NULL; /* Last Address at Capacity */
static void *brk_max     = NULL; /* Last Address at Max Capacity */

int mm_init(void) {
    /* Start right after the .data segment ends. */
    int base = (int)(&end);
    /* Longword align. */
    base = (base + (MM_ALIGNMENT - 1)) & -MM_ALIGNMENT;
    /* Set current position and start to base. */  
    brk_current = brk_start = (void *)base;
    /* Set end position and max to stack_end - 1. */
    brk_end = brk_max = (void *)(_arch_mem_top - MM_KERNEL_STACK_SIZE - 1);

    /* Newly claimed memory from sbrk() should be 
       zero-initialized the first time it is requested! */
    memset(brk_start, 0, brk_max - brk_start);

    return 0;
}

void *mm_brk_start(void) {
    return brk_start;
}

void *mm_brk_end(void) {
    return brk_end;
}

size_t mm_brk_free(void) {
    return brk_end - brk_current;
}

size_t mm_brk_size(void) {
    return brk_current - brk_start; 
}

size_t mm_brk_capacity(void) {
    return brk_end - brk_start;
}

size_t mm_brk_max_capacity(void) {
    return brk_max - brk_start;
}

int mm_brk_set_size(size_t bytes) {
    return mm_brk((void *)((size_t)brk_start + bytes));
}

int mm_brk_set_capacity(size_t bytes) {
    if(bytes > mm_brk_max_capacity())
        return -1;
    
    if(bytes < mm_brk_size())
        return -2;

    const int irqs = irq_disable();
    brk_end = (void*)((size_t)brk_start + bytes);
    irq_restore(irqs);

    return 0;
}

size_t mm_unused_capacity(void) {
    return brk_max - brk_end;
}

void *mm_unused_start(void) {
    return mm_unused_capacity()? (char *)mm_brk_end() + 1 : NULL;
}

void *mm_unused_end(void) {
    return mm_unused_capacity()? brk_max : NULL;
}

int mm_brk(void *new_end) {
    void *ret = mm_sbrk(new_end - brk_end);
    return ret == (void *)-1? -1 : 0;
}

void *mm_sbrk(intptr_t increment) {
    if(!increment)
        return brk_current;

    if(increment & (MM_ALIGNMENT - 1))
        increment = (increment + MM_ALIGNMENT) & ~(MM_ALIGNMENT - 1);

    const int irqs = irq_disable();

    void *current = brk_current;
    brk_current = (void *)((uintptr_t)brk_current + increment);

    if(brk_current > brk_end || brk_current < brk_start) {
        errno = ENOMEM;

        brk_current = current;
        current = (void *)-1;
        
        dbglog(DBG_WARNING, 
               "Out of memory! [Requested: %d, Free: %u, Used: %u]\n",
               increment, mm_brk_free(), mm_brk_size());
    } 

    irq_restore(irqs);

    return current;
}

void *mm_ram_start(void) {
    return (void *)_arch_mem_bottom;
}

void *mm_ram_end(void) {
    return (void *)_arch_mem_top;
}

size_t mm_ram_used(void) {

}

size_t mm_ram_free(void) {

}

void *mm_stack_start(void) {

}

void *mm_stack_current(void) {

}

void *mm_stack_free(void) {

}

