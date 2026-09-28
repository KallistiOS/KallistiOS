/* KallistiOS ##version##

   asid.c
   Copyright (C) 2026 Bruceleeto
*/

#include <kos.h>
#include <malloc.h>

/* This sample shows how to use several MMU contexts with different ASIDs.

   Two contexts (ASID 1 and 2) map the same virtual address to two
   different buffers. Switching context only changes the current ASID:
   the TLB can hold entries for both at once, so nothing has to be
   remapped or flushed, and the same pointer now reads the other buffer.

   The views are copied to the framebuffer with the store queues, which
   also checks that the SQ TLB entries are shared between all ASIDs. */

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

#define WIDTH   640
#define HEIGHT  480
#define FB_SIZE (WIDTH * HEIGHT * sizeof(uint16_t))

/* The virtual address both contexts use for their view */
#define VIEW_ADDR   0x10000000
#define VIEW        ((uint16_t *)VIEW_ADDR)

static void switch_to(mmucontext_t *cxt) {
    /* The TLB miss handler loads pages from the table in use, tagged with
       its ASID; switching the context sets the current ASID. */
    mmu_use_table(cxt);
    mmu_switch_context(cxt);
}

static uint16_t *map_view(mmucontext_t *cxt) {
    uint16_t *buf;

    buf = memalign(PAGESIZE, FB_SIZE);

    if(!buf)
        return NULL;

    /* We only access the buffer through the uncached mapping, so make sure
       no stale cache lines get written back over it later. */
    dcache_inval_range((uintptr_t)buf, FB_SIZE);

    mmu_page_map(cxt, VIEW_ADDR >> PAGESIZE_BITS,
                 ((uintptr_t)buf & 0x1fffffff) >> PAGESIZE_BITS,
                 FB_SIZE >> PAGESIZE_BITS,
                 MMU_ALL_RDWR, MMU_NO_CACHE, false, true);

    return buf;
}

static void draw_view(int asid) {
    char str[64];
    uint16_t x, y;

    /* Note that this always draws to the same address */
    for(y = 0; y < HEIGHT; y++) {
        for(x = 0; x < WIDTH; x++) {
            uint8_t v;

            if(asid == 1)
                v = (x * x + y * y) & 0xff;
            else
                v = (x ^ y) & 0xff;

            if(v >= 128)
                v = 127 - (v - 128);

            if(asid == 1)
                VIEW[y * WIDTH + x] = ((v >> 3) << 11)
                                      | ((v >> 2) << 5)
                                      | ((v >> 3) << 0);
            else
                VIEW[y * WIDTH + x] = (v >> 2) << 5;
        }
    }

    snprintf(str, sizeof(str), "ASID %d at %p", asid, (void *)VIEW);
    bfont_draw_str(VIEW + 20 * WIDTH + 20, WIDTH, 0, str);
    bfont_draw_str(VIEW + 50 * WIDTH + 20, WIDTH, 0,
                   "Press A to switch, START to exit");
}

int main(int argc, char **argv) {
    mmucontext_t *cxt[2] = { NULL, NULL };
    uint16_t *buf[2] = { NULL, NULL };
    uint32_t prev = 0;
    bool done = false;
    int cur = 0, i, rv = 0;

    /* Initialize MMU support */
    mmu_init();

    /* Setup two contexts that map the same virtual address to
       different buffers, and draw something different in each one */
    for(i = 0; i < 2; i++) {
        cxt[i] = mmu_context_create(i + 1);

        if(!cxt[i] || !(buf[i] = map_view(cxt[i]))) {
            fprintf(stderr, "Unable to create context %d\n", i + 1);
            rv = 1;
            goto out;
        }

        switch_to(cxt[i]);
        draw_view(i + 1);
    }

    switch_to(cxt[cur]);
    sq_cpy(vram_s, VIEW, FB_SIZE);

    while(!done) {
        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)

        if(st->buttons & CONT_START)
            done = true;

        /* Switch the ASID, then copy the other view to the framebuffer
           through the very same pointer */
        if((st->buttons & CONT_A) && !(prev & CONT_A)) {
            cur ^= 1;
            switch_to(cxt[cur]);
            sq_cpy(vram_s, VIEW, FB_SIZE);
        }

        prev = st->buttons;

        MAPLE_FOREACH_END()

        vid_waitvbl();
    }

out:
    /* Destroy the contexts */
    mmu_use_table(NULL);

    for(i = 0; i < 2; i++) {
        if(cxt[i])
            mmu_context_destroy(cxt[i]);

        free(buf[i]);
    }

    /* Shutdown MMU support */
    mmu_shutdown();

    return rv;
}
