/* KallistiOS ##version##

   nullptr.c
   (c)2002 Megan Potter
*/

#include <kos.h>

/* This shows how to setup a very simple MMU page mapping handler. When a
   TLB miss of some kind occurs and a page mapping is necessary, this
   handler will be called. The old handler is available as the return
   value from mmu_map_set_callback(), so this lets you chain handlers
   for handling certain regions specially. */

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

mmupage_t * catchnull(mmucontext_t *, int vp) {
    printf("Caught us trying to use a bad pointer!\n");
    printf("The pointer page was %08x\n", vp << PAGESIZE_BITS);
    printf("The address of the attempt was %08lx\n",
           CONTEXT_PC(thd_current->context));
    return NULL;
}

int main(int argc, char **argv) {
    /* Initialize MMU support */
    mmu_init();

    /* Set our handler */
    mmu_map_set_callback(catchnull);

/* Make sure the compiler doesn't complain about the bad thing 
we are doing intentionally */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-null-dereference"

    /* Do something naughty; note that this will actually work
       for both reads and writes in this case, but we can only
       really effectively test one ;) */
    *((uint16_t *)NULL) = 0;

/* Turn the warning back on */
#pragma GCC diagnostic pop

    /* We shouldn't get here... */

    /* Shutdown MMU support */
    mmu_shutdown();

    return 0;
}
