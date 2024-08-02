/* KallistiOS ##version##

   mm_tlsf.c
   Copyright (C) 2024 Paul Cercueil
*/

#include <kos/malloc_tlsf.h>
#include <kos/dbglog.h>

extern unsigned long _arch_mem_top;
extern unsigned long end;

int mm_init(void) {
    unsigned long end_addr = (unsigned long)&end;

    /* Align to 32 bytes */
    end_addr = (end_addr + 31) & ~0x1f;

    /* Use the space from the end of the executable all the way to the top
     * of the RAM. */
    kos_tlsf_init((void *)end_addr, (void *)(_arch_mem_top - 65536));

    return 0;
}

void mm_shutdown(void) {
    kos_tlsf_shutdown();
}

void *mm_sbrk(unsigned long increment) {
    return NULL;
}
