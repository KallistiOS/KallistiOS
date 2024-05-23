/* KallistiOS ##version##

   pvrmap.c
   (c)2002 Megan Potter
*/

#include <kos.h>

/* This sample doesn't particularly do anything helpful, but it shows off
   the basic usage of the built-in page table functions for remapping
   regions of memory. That main drawing code should certainly pull
   a double-take from any good coder ;) */

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

int main(int argc, char **argv) {
    mmucontext_t * cxt;
    uint16_t * vr;
    bool done = false;
    uint16_t x, y = 0;

    /* Initialize MMU support */
    mmu_init();

    /* Setup a context */
    cxt = mmu_context_create(0);
    mmu_use_table(cxt);
    mmu_switch_context(cxt);

    /* Map the PVR video memory to 0 */
    mmu_page_map(cxt, 0, 0x05000000 >> PAGESIZE_BITS, (8 * 1024 * 1024) >> PAGESIZE_BITS,
                 MMU_ALL_RDWR, MMU_NO_CACHE, 0, 1);

    /* Draw a nice pattern to the NULL space */
    vr = NULL;

/* Make sure the compiler doesn't complain about the bad thing 
we are doing intentionally */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-null-dereference"

    for(y = 0; y < 480; y++) {
        for(x = 0; x < 640; x++) {
            uint8_t v = ((x * x + y * y) & 0xff);

            if(v >= 128)
                v = 127 - (v - 128);

            vr[y * 640 + x] = ((v >> 3) << 11)
                              | ((v >> 2) << 5)
                              | ((v >> 3) << 0);
        }
    }

/* Turn the warning back on */
#pragma GCC diagnostic pop

    /* Draw some text */
    bfont_draw_str(vr + 20 * 640 + 20, 640, 0, "Press START!");

    /* Wait for start */
    while(!done) {
        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)

        if(st->buttons & CONT_START)
            done = true;

        MAPLE_FOREACH_END()
    }

    /* Destroy the context */
    mmu_context_destroy(cxt);

    /* Shutdown MMU support */
    mmu_shutdown();

    return 0;
}


