/* KallistiOS ##version##

   pvr2.c
   Copyright (C) 2026 Ruslan Rostovtsev
*/

#include <dc/elan.h>
#include <dc/pvr.h>
#include <kos/irq.h>

#define PVR2_FB_CFG_OFF         0x00800000
#define PVR2_SDRAM_CFG_VAL      0x15d1c955
#define PVR2_SDRAM_ARB_CFG_VAL  0x0000001f

static int pvr2_initted;

int pvr2_init(void) {
    if(!elan_present()) {
        return -1;
    }
    if(pvr2_initted) {
        return 0;
    }

    irq_disable_scoped();
    elan_pvr2_enable();

    PVR2_SET(PVR_RESET, PVR_RESET_ALL);
    PVR2_SET(PVR_RESET, PVR_RESET_NONE);
    PVR2_SET(PVR_FB_CFG_1, PVR2_FB_CFG_OFF);
    PVR2_SET(PVR_SDRAM_CFG, PVR2_SDRAM_CFG_VAL);
    PVR2_SET(PVR_SDRAM_REFRESH, 0x00000020);
    PVR2_SET(PVR_SDRAM_ARB_CFG, PVR2_SDRAM_ARB_CFG_VAL);
    PVR2_SET(PVR_FPU_PARAM_CFG, 0x0027df77);
    PVR2_SET(PVR_HALF_OFFSET, 0x00000007);
    PVR2_SET(PVR_ISP_FEED_CFG, 0x00800408);
    PVR2_SET(PVR_FB_BURSTCTRL, 0x00093f39);
    PVR2_SET(PVR_Y_COEFF, 0x00008040);

    pvr2_initted = 1;
    return 0;
}

int pvr2_shutdown(void) {
    if(!pvr2_initted) {
        return -1;
    }

    irq_disable_scoped();
    PVR2_SET(PVR_RESET, PVR_RESET_ALL);
    PVR2_SET(PVR_RESET, PVR_RESET_NONE);
    elan_pvr2_disable();
    pvr2_initted = 0;
    return 0;
}

int pvr2_enabled(void) {
    return pvr2_initted;
}
