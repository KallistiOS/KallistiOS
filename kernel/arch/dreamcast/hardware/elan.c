/* KallistiOS ##version##

   elan.c
   Copyright (C) 2026 Ruslan Rostovtsev
*/

#include <arch/arch.h>
#include <dc/elan.h>

bool elan_present(void) {
    if(hardware_sys_mode(NULL) != HW_TYPE_NAOMI) {
        return false;
    }
    return ELAN_GET(ELAN_REG_ID) == ELAN_MAGIC;
}

uint32_t elan_revision(void) {
    if(!elan_present()) {
        return 0;
    }
    return ELAN_GET(ELAN_REG_REVISION);
}

uint32_t elan_ifctl_get(void) {
    if(!elan_present()) {
        return 0;
    }
    return ELAN_GET(ELAN_REG_IFCTL);
}

void elan_ifctl_set(uint32_t value) {
    if(!elan_present()) {
        return;
    }
    ELAN_SET(ELAN_REG_IFCTL, value);
}

void elan_pvr2_enable(void) {
    elan_ifctl_set(ELAN_IFCTL_PVR2);
}

void elan_pvr2_disable(void) {
    elan_ifctl_set(0);
}

bool elan_pvr2_enabled(void) {
    return (elan_ifctl_get() & ELAN_IFCTL_PVR2) != 0;
}
