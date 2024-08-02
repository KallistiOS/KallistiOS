/* KallistiOS ##version##

   malloc.c
   Copyright (C) 2024 Paul Cercueil
*/

#include <stdlib.h>
#include <kos/malloc.h>

#ifdef KOS_USE_TLSF
#  include <kos/malloc_tlsf.h>
#  define KOS_MM_IMPL(fn) kos_tlsf_##fn
#else
#  include <kos/dlmalloc.h>
#  define KOS_MM_IMPL(fn) dl##fn
#endif

void free(void *ptr) {
    KOS_MM_IMPL(free)(ptr);
}

void *malloc(size_t size) {
    return KOS_MM_IMPL(malloc)(size);
}

void *calloc(size_t nmemb, size_t size) {
    return KOS_MM_IMPL(calloc)(nmemb, size);
}

void *realloc(void *ptr, size_t size) {
    return KOS_MM_IMPL(realloc)(ptr, size);
}

void *memalign(size_t alignment, size_t size) {
    return KOS_MM_IMPL(memalign)(alignment, size);
}

void malloc_stats(void) {
    KOS_MM_IMPL(malloc_stats)();
}

int malloc_irq_safe(void) {
    return KOS_MM_IMPL(malloc_irq_safe)();
}
