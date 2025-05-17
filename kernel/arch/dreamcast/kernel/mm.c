/* KallistiOS ##version##

   mm.c
   (c)2000-2001 Megan Potter
*/

/* Defines a simple UNIX-style memory pool system. Since the Dreamcast has
   multiple distinct areas of memory used for different things, we'll
   want to keep separate pools. Mainly this will be used with the PowerVR
   and the system RAM, since the SPU has its own program (that can do its
   own memory management). */


/* Note: right now we only support system RAM */


#include <arch/types.h>
#include <arch/arch.h>
#include <arch/irq.h>
#include <kos/dbglog.h>
#include <errno.h>
#include <stdio.h>

/* The end of the program is always marked by the '_end' symbol. So we'll
   longword-align that and add a little for safety. sbrk() calls will
   move up from there. */
extern unsigned long end;
static void *sbrk_base;

/* MM-wide initialization */
int mm_init(void) {
    int base = (int)(&end);
    base = (base / 4) * 4 + 4;  /* longword align */
    sbrk_base = (void*)base;

    return 0;
}

/* Simple sbrk function */
void* mm_sbrk(unsigned long increment) {
    int old;
    void *base = sbrk_base;

    old = irq_disable();

    if(increment & 3)
        increment = (increment + 4) & ~3;

    sbrk_base = (void *)(increment + (unsigned long)sbrk_base);

    if(((uint32)sbrk_base) >= (_arch_mem_top - THD_KERNEL_STACK_SIZE)) {
        dbglog(DBG_CRITICAL, "Out of memory. Requested sbrk_base %p, was %p, diff %lu\n",
               sbrk_base, base, increment);
        sbrk_base = base;  /* Restore old value and mark failed */
        errno = ENOMEM;
        irq_restore(old);
        return (void*) -1;
    }

    irq_restore(old);

    return base;
}
