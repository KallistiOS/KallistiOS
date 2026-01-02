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

#include <arch/arch.h>
#include <arch/stack.h>
#include <kos/dbglog.h>
#include <kos/mm.h>
#include <kos/linker.h>
#include <errno.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

/* The end of the program is always marked by the 'end' symbol. So we'll
   just longword-align that. sbrk() calls will move up from there. */
static uintptr_t sbrk_base;

/* MM-wide initialization */
int mm_init(void) {
    uintptr_t base = (uintptr_t)end;
    base = __align_up(base, 4);
    sbrk_base = base;

    return 0;
}

/* Simple sbrk function */
void *mm_sbrk(size_t increment) {
    uintptr_t base = sbrk_base;
    uintptr_t new_base;

    increment = __align_up(increment, 4);

    do {
        new_base = base + increment;

        if(new_base >= (_arch_mem_top - THD_KERNEL_STACK_SIZE)) {
            dbglog(DBG_CRITICAL, "Out of memory. Requested sbrk_base %" PRIxPTR \
                   ", was %" PRIxPTR ", diff %zu\n", new_base, base, increment);
            errno = ENOMEM;
            return (void *)-1;
        }
    } while(!atomic_compare_exchange_strong(&sbrk_base, &base, new_base));

    return (void *)base;
}
