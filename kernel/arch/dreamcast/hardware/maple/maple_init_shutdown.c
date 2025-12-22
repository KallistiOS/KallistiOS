/* KallistiOS ##version##

   maple_init.c
   Copyright (C) 2002 Megan Potter
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <dc/memory.h>
#include <dc/maple.h>
#include <dc/asic.h>
#include <dc/vblank.h>
#include <kos/thread.h>
#include <kos/init.h>
#include <kos/dbglog.h>

#include <dc/maple/controller.h>
#include <dc/maple/keyboard.h>
#include <dc/maple/mouse.h>
#include <dc/maple/vmu.h>
#include <dc/maple/purupuru.h>
#include <dc/maple/sip.h>
#include <dc/maple/dreameye.h>
#include <dc/maple/lightgun.h>

/*
  This system handles low-level communication/initialization of the maple
  bus.  Specific devices aren't handled by this module, rather, the modules
  implementing specific devices can use this module to access them.

  Thanks to Marcus Comstedt for information on the maple bus.
  Thanks to the LinuxDC guys for some ideas on how to better implement this.

*/

/* Initialize Hardware (call after driver inits) */
static void maple_hw_init(void) {
    maple_driver_t *drv;
    int p, u;

    dbglog(DBG_INFO, "maple: active drivers:\n");

    /* Reset structures */
    for(p = 0; p < MAPLE_PORT_COUNT; p++) {
        maple_state.ports[p].port = p;

        for(u = 0; u < MAPLE_UNIT_COUNT; u++)
            maple_state.ports[p].units[u] = NULL;
    }

    TAILQ_INIT(&maple_state.frame_queue);

    /* Enumerate drivers */
    LIST_FOREACH(drv, &maple_state.driver_list, drv_list) {
        dbglog(DBG_INFO, "    %s: %s\n", drv->name, maple_pcaps(drv->functions));
    }

    /* Allocate the DMA send buffer */
    if(__is_defined(MAPLE_DMA_DEBUG))
        maple_state.dma_buffer = aligned_alloc(32, MAPLE_DMA_SIZE + 1024);
    else
        maple_state.dma_buffer = aligned_alloc(32, MAPLE_DMA_SIZE);

    assert_msg(maple_state.dma_buffer != NULL, "Couldn't allocate maple DMA buffer");
    assert_msg((((uint32)maple_state.dma_buffer) & 0x1f) == 0, "DMA buffer was unaligned; bug in dlmalloc; please report!");

    /* Force it into the P2 area */
    maple_state.dma_buffer = (uint8*)((((uint32)maple_state.dma_buffer) & MEM_AREA_CACHE_MASK) | MEM_AREA_P2_BASE);

    if(__is_defined(MAPLE_DMA_DEBUG)) {
        maple_state.dma_buffer += 512;
        maple_sentinel_setup(maple_state.dma_buffer - 512, MAPLE_DMA_SIZE + 1024);
    }

    maple_state.dma_in_progress = 0;
    dbglog(DBG_INFO, "  DMA Buffer at %08lx\n", (uint32)maple_state.dma_buffer);

    /* Initialize other misc stuff */
    maple_state.vbl_cntr = maple_state.dma_cntr = 0;
    maple_state.detect_port_next = 0;
    maple_state.scan_ready_mask = 0;
    maple_state.gun_port = -1;
    maple_state.gun_x = maple_state.gun_y = -1;

    /* Reset hardware */
    maple_write(MAPLE_RESET1, MAPLE_RESET1_MAGIC);
    maple_write(MAPLE_RESET2, MAPLE_RESET2_MAGIC);
    maple_write(MAPLE_SPEED, MAPLE_SPEED_2MBPS | MAPLE_SPEED_TIMEOUT(50000));
    maple_bus_enable();

    /* Hook the necessary interrupts */
    maple_state.vbl_handle = vblank_handler_add(maple_vbl_irq_hnd, &maple_state);
    asic_evt_set_handler(ASIC_EVT_MAPLE_DMA, maple_dma_irq_hnd, &maple_state);
    asic_evt_enable(ASIC_EVT_MAPLE_DMA, ASIC_IRQ_DEFAULT);
}

/* Turn off the maple bus, free mem */
/* AGGG!! Someone save me from this idiotic voodoo bug fixing crap.. */
void maple_hw_shutdown(void) {
    int p, u, cnt;
    uint32  ptr;
    maple_device_t *dev;

    /* Unhook interrupts */
    vblank_handler_remove(maple_state.vbl_handle);
    asic_evt_remove_handler(ASIC_EVT_MAPLE_DMA);
    asic_evt_disable(ASIC_EVT_MAPLE_DMA, ASIC_IRQ_DEFAULT);

    /* Stop any existing maple DMA and shut down the bus */
    maple_dma_stop();

    while(maple_dma_in_progress())
        ;

    maple_bus_disable();

    /* We must cast this back to P1 or cache issues will arise */
    if(maple_state.dma_buffer != NULL) {
        ptr = (uint32)maple_state.dma_buffer;

        if(__is_defined(MAPLE_DMA_DEBUG))
            ptr -= 512;

        ptr = (ptr & MEM_AREA_CACHE_MASK) | MEM_AREA_P1_BASE;
        free((void *)ptr);
        maple_state.dma_buffer = NULL;
    }

    /* Free any attached devices */
    for(cnt = 0, p = 0; p < MAPLE_PORT_COUNT; p++) {
        for(u = 0; u < MAPLE_UNIT_COUNT; u++) {
            cnt += !!maple_driver_detach(p, u);

            dev = maple_state.ports[p].units[u];
            if(dev)
                free(dev->status);
            free(dev);
        }
    }

    dbglog(DBG_DEBUG, "maple: final stats -- device count = %d, vbl_cntr = %d, dma_cntr = %d\n",
           cnt, maple_state.vbl_cntr, maple_state.dma_cntr);
}

/* Wait for the initial bus scan to complete */
void maple_wait_scan(void) {
    int     p, u;
    maple_device_t  *dev;

    /* Wait for it to finish */
    while(maple_state.scan_ready_mask != 0xf)
        thd_pass();

    /* Enumerate everything */
    dbglog(DBG_INFO, "maple: attached devices:\n");

    for(p = 0; p < MAPLE_PORT_COUNT; p++) {
        for(u = 0; u < MAPLE_UNIT_COUNT; u++) {
            dev = maple_enum_dev(p, u);

            if(dev) {
                dbglog(DBG_INFO, "  %c%c: %.30s (%08lx: %s)\n",
                       'A' + p, '0' + u,
                       dev->info.product_name,
                       dev->info.functions, maple_pcaps(dev->info.functions));
            }
        }
    }
}

KOS_INIT_FLAG_WEAK(cont_init, true);
KOS_INIT_FLAG_WEAK(kbd_init, true);
KOS_INIT_FLAG_WEAK(mouse_init, true);
KOS_INIT_FLAG_WEAK(lightgun_init, true);
KOS_INIT_FLAG_WEAK(vmu_init, true);
KOS_INIT_FLAG_WEAK(purupuru_init, true);
KOS_INIT_FLAG_WEAK(sip_init, true);
KOS_INIT_FLAG_WEAK(dreameye_init, true);

/* Full init: initialize known drivers and start maple operations */
void maple_init(void) {
    KOS_INIT_FLAG_CALL(lightgun_init);
    KOS_INIT_FLAG_CALL(cont_init);
    KOS_INIT_FLAG_CALL(kbd_init);
    KOS_INIT_FLAG_CALL(mouse_init);
    KOS_INIT_FLAG_CALL(vmu_init);
    KOS_INIT_FLAG_CALL(purupuru_init);
    KOS_INIT_FLAG_CALL(sip_init);
    KOS_INIT_FLAG_CALL(dreameye_init);

    maple_hw_init();
}

KOS_INIT_FLAG_WEAK(cont_shutdown, true);
KOS_INIT_FLAG_WEAK(kbd_shutdown, true);
KOS_INIT_FLAG_WEAK(mouse_shutdown, true);
KOS_INIT_FLAG_WEAK(lightgun_shutdown, true);
KOS_INIT_FLAG_WEAK(vmu_shutdown, true);
KOS_INIT_FLAG_WEAK(purupuru_shutdown, true);
KOS_INIT_FLAG_WEAK(sip_shutdown, true);
KOS_INIT_FLAG_WEAK(dreameye_shutdown, true);

/* Full shutdown: shutdown maple operations and known drivers */
void maple_shutdown(void) {
    maple_hw_shutdown();

    KOS_INIT_FLAG_CALL(dreameye_shutdown);
    KOS_INIT_FLAG_CALL(sip_shutdown);
    KOS_INIT_FLAG_CALL(purupuru_shutdown);
    KOS_INIT_FLAG_CALL(vmu_shutdown);
    KOS_INIT_FLAG_CALL(mouse_shutdown);
    KOS_INIT_FLAG_CALL(kbd_shutdown);
    KOS_INIT_FLAG_CALL(cont_shutdown);
    KOS_INIT_FLAG_CALL(lightgun_shutdown);
}
