/* KallistiOS ##version##

   hardware.c
   Copyright (C) 2000-2001 Megan Potter
   Copyright (C) 2013 Lawrence Sebald
*/

#include <stdbool.h>
#include <arch/arch.h>
#include <kos/init.h>
#include <kos/platform.h>
#include <dc/spu.h>
#include <dc/video.h>
#include <dc/cdrom.h>
#include <dc/asic.h>
#include <dc/maple.h>
#include <dc/net/broadband_adapter.h>
#include <dc/net/lan_adapter.h>
#include <dc/vblank.h>

static int initted = 0;

#define SYSMODE_REG 0xA05F74B0

int hardware_sys_mode(int *region) {
    uint32 sm = *((vuint32 *)SYSMODE_REG);

    if(region)
        *region = sm & 0x0F;

    return (sm >> 4) & 0x0F;
}

int hardware_sys_init(void) {
    /* Setup ASIC stuff */
    asic_init();

    /* VBlank multiplexer */
    vblank_init();

    initted = 1;

    return 0;
}


void bba_la_init(void) {
    /* Setup network (this won't do anything unless we enable netcore) */
    bba_init();
    la_init();
}

void bba_la_shutdown(void) {
    la_shutdown();
    bba_shutdown();
}

KOS_INIT_FLAG_WEAK(bba_la_init, false);
KOS_INIT_FLAG_WEAK(bba_la_shutdown, false);
KOS_INIT_FLAG_WEAK(maple_init, true);
KOS_INIT_FLAG_WEAK(cdrom_init, true);
KOS_INIT_FLAG_WEAK(cdrom_shutdown, true);

int hardware_periph_init(void) {
    /* Init sound */
    spu_init();
    g2_dma_init();

    if(!KOS_PLATFORM_IS_NAOMI) {
        /* Init CD-ROM.. NOTE: NO GD-ROM SUPPORT. ONLY CDs/CDRs. */
        KOS_INIT_FLAG_CALL(cdrom_init);
    }

    /* Setup maple bus */
    KOS_INIT_FLAG_CALL(maple_init);

    /* Init video */
    vid_init(DEFAULT_VID_MODE, DEFAULT_PIXEL_MODE);

    if(!KOS_PLATFORM_IS_NAOMI)
        KOS_INIT_FLAG_CALL(bba_la_init);

    initted = 2;

    return 0;
}

KOS_INIT_FLAG_WEAK(maple_shutdown, true);

void hardware_shutdown(void) {
    switch(initted) {
        case 2:
            if(!KOS_PLATFORM_IS_NAOMI)
                KOS_INIT_FLAG_CALL(bba_la_shutdown);
            KOS_INIT_FLAG_CALL(maple_shutdown);
            if(!KOS_PLATFORM_IS_NAOMI)
                KOS_INIT_FLAG_CALL(cdrom_shutdown);
            g2_dma_shutdown();
            spu_shutdown();
            vid_shutdown();
            /* fallthru */
        case 1:
            vblank_shutdown();
            asic_shutdown();
            /* fallthru */
        case 0:
            return;
    }
}
