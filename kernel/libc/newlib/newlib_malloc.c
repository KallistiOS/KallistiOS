/* KallistiOS ##version##

   newlib_malloc.c
   Copyright (C) 2004 Megan Potter

*/

#include <stdlib.h>
#include <reent.h>

void *memalign(size_t alignment, size_t size);

// We have to provide these for Newlib's reent pieces.

__used void _free_r(struct _reent *re, void *ptr) {
    (void)re;
    free(ptr);
}

__used void *_malloc_r(struct _reent *re, size_t amt) {
    (void)re;
    return malloc(amt);
}

__used void *_calloc_r(struct _reent *re, size_t nmemb, size_t size) {
    (void)re;
    return calloc(nmemb, size);
}

__used void *_realloc_r(struct _reent *re, void *ptr, size_t size) {
    (void)re;
    return realloc(ptr, size);
}

__used void *_memalign_r(struct _reent *re, size_t align, size_t nbytes) {
    (void)re;
    return memalign(align, nbytes);
}
