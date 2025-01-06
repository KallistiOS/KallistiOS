/* KallistiOS ##version##

   pvr_mem.c
   Copyright (C) 2002 Megan Potter

 */

#include <assert.h>
#include <dc/pvr.h>
#include "pvr_internal.h"
#include <stdio.h>

#include <malloc.h> /* For the struct mallinfo defs */

#include <kos/opts.h>

/*

This module basically serves as a KOS-friendly front end and support routines
for the pvr_mem_core module, which is a dlmalloc-derived malloc for use with
the PVR memory pool.

I was originally going to make a totally separate thing that could be used
to generically manage any memory pool, but then I realized what a gruelling
and thankless task that would be when starting with dlmalloc, so we have this
instead. ^_^;

*/

/* Bring in some prototypes from pvr_mem_core.c */
/* We can't directly include its header because of name clashes with
   the real malloc header */
extern void * pvr_int_malloc(size_t bytes);
extern void pvr_int_free(void *ptr);
extern struct mallinfo pvr_int_mallinfo();
extern void pvr_int_mem_reset();
extern void pvr_int_malloc_stats();


#ifdef PVR_KM_DBG

#include <kos/thread.h>
#include <arch/arch.h>

/* List of allocated memory blocks for leak checking */
typedef struct memctl {
    uint32          size;
    tid_t           thread;
    uint32          addr;
    pvr_ptr_t       block;
    LIST_ENTRY(memctl)  list;
} memctl_t;

static LIST_HEAD(memctl_list, memctl) block_list;

#endif  /* PVR_KM_DBG */


/* PVR RAM base; NULL is considered invalid */
static pvr_ptr_t pvr_mem_base = NULL;
#define CHECK_MEM_BASE \
    assert_msg(pvr_mem_base != NULL, \
               "pvr_mem_* used, but PVR hasn't been initialized yet")

/* Used in pvr_mem_core.c */
void * pvr_int_sbrk(size_t amt) {
    uint32 old, n;

    /* Are we valid? */
    CHECK_MEM_BASE;

    /* Try to increment it */
    old = (uint32)pvr_mem_base;
    n = old + amt;

    /* Did we run over? */
    if(n > PVR_RAM_INT_TOP)
        return (void *) - 1;

    /* Nope, everything's cool */
    pvr_mem_base = (pvr_ptr_t)n;
    return (pvr_ptr_t)old;
}

/* Allocate a chunk of memory from texture space; the returned value
   will be relative to the base of texture memory (zero-based) */
pvr_ptr_t pvr_mem_malloc(size_t size) {
    uint32 rv32;
#ifdef PVR_KM_DBG
    uint32      ra = arch_get_ret_addr();
    memctl_t    * ctl;
#endif  /* PVR_KM_DBG */

    CHECK_MEM_BASE;

    rv32 = (uint32)pvr_int_malloc(size);
    assert_msg((rv32 & 0x1f) == 0,
               "dlmalloc's alignment is broken; "
               "please make a bug report");

#ifdef PVR_KM_DBG
    ctl = malloc(sizeof(memctl_t));
    ctl->size = size;
    ctl->thread = thd_current->tid;
    ctl->addr = ra;
    ctl->block = (pvr_ptr_t)rv32;
    LIST_INSERT_HEAD(&block_list, ctl, list);
#endif  /* PVR_KM_DBG */

#ifdef PVR_KM_DBG_VERBOSE
    printf("Thread %d/%08lx allocated %lu bytes at %08lx\n",
           ctl->thread, ctl->addr, ctl->size, rv32);
#endif

    return (pvr_ptr_t)rv32;
}

/* Free a previously allocated chunk of memory */
void pvr_mem_free(pvr_ptr_t chunk) {
#ifdef PVR_KM_DBG
    uint32      ra = arch_get_ret_addr();
    memctl_t    *ctl, *tmp;
    int     found;
#endif  /* PVR_KM_DBG */

    CHECK_MEM_BASE;

#ifdef PVR_KM_DBG_VERBOSE
    printf("Thread %d/%08lx freeing block @ %08lx\n",
           thd_current->tid, ra, (uint32)chunk);
#endif

#ifdef PVR_KM_DBG
    found = 0;

    LIST_FOREACH_SAFE(ctl, &block_list, list, tmp) {
        if(ctl->block == chunk) {
            LIST_REMOVE(ctl, list);
            free(ctl);
            found = 1;
            break;
        }
    }

    if(!found) {
        dbglog(DBG_ERROR, 
               "pvr_mem_free: trying to free non-alloc'd block "
               "%08lx (called from %d/%08lx\n",
               (uint32)chunk, thd_current->tid, ra);
    }

#endif  /* PVR_KM_DBG */

    pvr_int_free((void *)chunk);
}

/* Check the memory block list to see what's allocated */
void pvr_mem_print_list(void) {
#ifdef PVR_KM_DBG
    memctl_t    * ctl;

    printf("pvr_mem_print_list block list:\n");
    LIST_FOREACH(ctl, &block_list, list) {
        printf("  unfreed block at %08lx size %lu, "
               "allocated by thread %d/%08lx\n",
               (unsigned long)ctl->block, ctl->size, 
               ctl->thread, (unsigned long)ctl->addr);
    }
    printf("pvr_mem_print_list end block list\n");
#endif  /* PVR_KM_DBG */
}

/* Return the number of bytes available still in the memory pool */
static size_t pvr_mem_available_int(void) {
    struct mallinfo mi = pvr_int_mallinfo();

    /* This magic formula is modeled after mstats() */
    return mi.arena - mi.uordblks;
}

size_t pvr_mem_available(void) {
    if(!pvr_mem_base)
        return 0;

    return pvr_mem_available_int() + 
        (PVR_RAM_INT_TOP - (size_t)pvr_mem_base);
}

/* Reset the memory pool, equivalent to freeing all textures currently
   residing in RAM. This _must_ be done on a mode change, configuration
   change, etc. */
void pvr_mem_reset(void) {
    if(!pvr_state.valid)
        pvr_mem_base = NULL;
    else {
        pvr_mem_base = (pvr_ptr_t)(PVR_RAM_INT_BASE + pvr_state.texture_base);
        pvr_int_mem_reset();
    }
}

/* Print some statistics (like mallocstats) */
void pvr_mem_stats(void) {
    printf("pvr_mem_stats():\n");
    pvr_int_malloc_stats();
    printf("max sbrk base: %08lx\n", (uint32)pvr_mem_base);
#ifdef PVR_KM_DBG
    pvr_mem_print_list();
#endif
}
