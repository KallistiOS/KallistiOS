/* KallistiOS ##version##

   malloc_tlsf.h
   Copyright (C) 2024 Paul Cercueil
*/

#ifndef __MALLOC_TLSF_H
#define __MALLOC_TLSF_H

#include <stdlib.h>

void kos_tlsf_free(void *ptr);

void *kos_tlsf_malloc(size_t size);
void *kos_tlsf_calloc(size_t nmemb, size_t size);
void *kos_tlsf_realloc(void *ptr, size_t size);
void *kos_tlsf_memalign(size_t align, size_t size);

void kos_tlsf_init(void *start, void *end);
void kos_tlsf_shutdown(void);

void kos_tlsf_malloc_stats(void);
int kos_tlsf_malloc_irq_safe(void);

#endif /* __MALLOC_TLSF_H */
