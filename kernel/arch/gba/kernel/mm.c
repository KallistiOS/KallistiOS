/* KallistiOS 2.0.0

   mm.c
   (c)2000-2001 Dan Potter
*/

/* Defines a simple UNIX-style memory pool system. */

#include <arch/types.h>
#include <arch/arch.h>

/* The end of the program is always marked by the '_end' symbol. So we'll
   longword-align that and add a little for safety. sbrk() calls will
   move up from there. */
extern unsigned long end;
static void *sbrk_base;

/* MM-wide initialization */
int mm_init() {
    int base = (int)(&end);
    base = (base / 4) * 4 + 4;  /* longword align */
    sbrk_base = (void*)base;

    return 0;
}

/* Simple sbrk function */
void* mm_sbrk(unsigned long increment) {
    void *base = sbrk_base;

    if(increment & 3)
        increment = (increment + 4) & ~3;

    sbrk_base += increment;

    if(((uint32)sbrk_base) >= (_arch_mem_top - 4096)) {
        panic("out of memory; about to run over kernel stack");
    }

    return base;
}


