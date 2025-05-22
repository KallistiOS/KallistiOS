/*
   AICAOS

   mm.c
   Copyright (C) 2025 Paul Cercueil

   Memory management routines
*/

#include <aicaos/init.h>
#include <aicaos/lock.h>

#include "tlsf/tlsf.h"
#include "tlsf/tlsf.c"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

static tlsf_t tlsf;
static struct mutex mm_lock = MUTEX_INITIALIZER;

extern unsigned int __heap_start, __heap_end;

static void aica_mm_init(void)
{
    unsigned int size = (unsigned int)&__heap_end - (unsigned int)&__heap_start;

    tlsf = tlsf_create_with_pool(&__heap_start, size);
}
aicaos_initcall(aica_mm_init);

void * malloc(size_t size)
{
    mutex_lock_scoped(&mm_lock);

    return tlsf_malloc(tlsf, size);
}

void * calloc(size_t nb, size_t size)
{
    void *ptr = malloc(nb * size);

    if (ptr)
        memset(ptr, 0, nb * size);

    return ptr;
}

void * memalign(size_t align, size_t size)
{
    mutex_lock_scoped(&mm_lock);

    return tlsf_memalign(tlsf, align, size);
}

void * realloc(void *ptr, size_t size)
{
    mutex_lock_scoped(&mm_lock);

    return tlsf_realloc(tlsf, ptr, size);
}

void free(void *ptr)
{
    mutex_lock_scoped(&mm_lock);

    tlsf_free(tlsf, ptr);
}

void _free_r(struct _reent *reent, void *ptr)
{
    (void)reent;

    free(ptr);
}

void * _malloc_r(struct _reent *reent, size_t size)
{
    (void)reent;

    return malloc(size);
}

void * _calloc_r(struct _reent *reent, size_t nb, size_t size)
{
    (void)reent;

    return calloc(nb, size);
}

void * _memalign_r(struct _reent *reent, size_t align, size_t size)
{
    (void)reent;

    return memalign(align, size);
}

void * _realloc_r(struct _reent *reent, void *ptr, size_t size)
{
    (void)reent;

    return realloc(ptr, size);
}

static void aica_sum_free(void *ptr, size_t size, int used, void *user)
{
    unsigned int *available = user;

    if (!used)
        *available += (unsigned int)size;
}

unsigned int mem_available(void)
{
    unsigned int available = 0;
    pool_t pool = tlsf_get_pool(tlsf);

    mutex_lock(&mm_lock);
    tlsf_walk_pool(pool, aica_sum_free, &available);
    mutex_unlock(&mm_lock);

    return available;
}
