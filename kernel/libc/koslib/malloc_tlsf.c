/* KallistiOS ##version##

   malloc_tlsf.c
   Copyright (C) 2024 Paul Cercueil
*/

#include "../tlsf/tlsf.h"

#include <kos/mutex.h>
#include <kos/malloc_tlsf.h>
#include <string.h>

#include <arch/irq.h>

static mutex_t tlsf_mutex = MUTEX_INITIALIZER;

static tlsf_t tlsf;

void kos_tlsf_free(void *ptr) {
    mutex_lock(&tlsf_mutex);
    tlsf_free(tlsf, ptr);
    mutex_unlock(&tlsf_mutex);
}

void * kos_tlsf_malloc(size_t bytes) {
    void *ret;

    mutex_lock(&tlsf_mutex);
    ret = tlsf_malloc(tlsf, bytes);
    mutex_unlock(&tlsf_mutex);

    return ret;
}

void * kos_tlsf_calloc(size_t nmemb, size_t size) {
    void *ret;

    size *= nmemb;

    mutex_lock(&tlsf_mutex);

    ret = tlsf_malloc(tlsf, size);
    if (ret)
        memset(ret, 0, size);

    mutex_unlock(&tlsf_mutex);

    return ret;
}

void * kos_tlsf_realloc(void *ptr, size_t size) {
    void *ret;

    mutex_lock(&tlsf_mutex);
    ret = tlsf_realloc(tlsf, ptr, size);
    mutex_unlock(&tlsf_mutex);

    return ret;
}

void * kos_tlsf_memalign(size_t align, size_t bytes) {
    void *ret;

    mutex_lock(&tlsf_mutex);
    ret = tlsf_memalign(tlsf, align, bytes);
    mutex_unlock(&tlsf_mutex);

    return ret;
}

void kos_tlsf_init(void *start, void *end) {
    tlsf = tlsf_create_with_pool(start, end - start);
}

void kos_tlsf_shutdown(void) {
    tlsf_destroy(tlsf);
}

void kos_tlsf_malloc_stats(void) {
}

int kos_tlsf_malloc_irq_safe(void) {
    return !mutex_is_locked(&tlsf_mutex);
}
