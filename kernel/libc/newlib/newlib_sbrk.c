/* KallistiOS ##version##

   newlib_sbrk.c
   Copyright (C)2004 Megan Potter

*/

#include <reent.h>
#include <kos/mm.h>

void *_sbrk_r(struct _reent *reent, ptrdiff_t incr) {
    (void)reent;
    return mm_sbrk(incr);
}
