/* KallistiOS ##version##

   dc/elan.h
   Copyright (C) 2026 Ruslan Rostovtsev

*/

/** \file    dc/elan.h
    \brief   VideoLogic Elan T&L coprocessor (NAOMI 2).
    \ingroup elan

    Elan is the geometry processor on NAOMI 2 and has 32 MB of local RAM.
    IFCTL bit 2 enables CLXB's register and VRAM windows; they are left
    disabled at boot.

    Call \ref pvr2_init() when CPU access to CLXB VRAM is required. Do not
    set \ref ELAN_IFCTL_CS1_BCAST unless you intend to broadcast CS1 writes
    to both GPUs.

    \author Ruslan Rostovtsev
*/

#ifndef __DC_ELAN_H
#define __DC_ELAN_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdbool.h>
#include <stdint.h>
#include <kos/regfield.h>
#include <dc/memory.h>

/** \defgroup elan  Elan
    \brief          VideoLogic Elan T&L coprocessor (NAOMI 2)
    \ingroup        video

    @{
*/

/** \brief  Elan register block, P2. */
#define ELAN_BASE               0xa8800000

/** \brief  Value of \ref ELAN_REG_ID on a present Elan. */
#define ELAN_MAGIC              0xe1ad0000

/** \brief  Elan local RAM, P0. */
#define ELAN_RAM_BASE_P0        0x0a000000
/** \brief  Elan local RAM, P2. */
#define ELAN_RAM_BASE           (MEM_AREA_P2_BASE | ELAN_RAM_BASE_P0)
/** \brief  Elan local RAM size in bytes. */
#define ELAN_RAM_SIZE           (32 * 1024 * 1024)

/** \defgroup elan_regs Register offsets
    \brief              Offsets from \ref ELAN_BASE
    \ingroup            elan

    @{
*/
#define ELAN_REG_ID             0x00    /**< \brief Magic (\ref ELAN_MAGIC) */
#define ELAN_REG_REVISION       0x04    /**< \brief Chip revision */
#define ELAN_REG_CMDQ           0x0c    /**< \brief Command queue occupancy */
#define ELAN_REG_IFCTL          0x10    /**< \brief SH-4 / PVR interface */
#define ELAN_REG_REFRESH        0x14    /**< \brief SDRAM refresh */
#define ELAN_REG_SDRAMCFG       0x1c    /**< \brief SDRAM config */
#define ELAN_REG_TILER          0x30    /**< \brief Macro tiler */
#define ELAN_REG_IRQSTAT        0x74    /**< \brief IRQ status */
#define ELAN_REG_IRQMASK        0x78    /**< \brief IRQ mask */
/** @} */

/** \defgroup elan_ifctl IFCTL bits
    \brief               Fields of \ref ELAN_REG_IFCTL
    \ingroup             elan

    @{
*/
#define ELAN_IFCTL_CS1_BCAST    BIT(0)  /**< \brief Broadcast writes on CS1 (both PVRs) */
#define ELAN_IFCTL_CH2          BIT(1)  /**< \brief Elan channel 2 */
#define ELAN_IFCTL_PVR2         BIT(2)  /**< \brief Enable CLXB address windows */
/** @} */

/** \brief  Read an Elan register. */
#define ELAN_GET(reg)           (*(volatile uint32_t *)(ELAN_BASE + (reg)))
/** \brief  Write an Elan register. */
#define ELAN_SET(reg, value)    (ELAN_GET(reg) = (value))

/** \brief  True when Elan is present (NAOMI 2). */
bool elan_present(void);

/** \brief  Elan revision register, or 0 if absent. */
uint32_t elan_revision(void);

/** \brief  Current IFCTL value, or 0 if Elan is absent. */
uint32_t elan_ifctl_get(void);

/** \brief  Write IFCTL.

    \warning
    Bit 0 (\ref ELAN_IFCTL_CS1_BCAST) broadcasts CS1 to both GPUs. Leave it
    clear unless you are duplicating textures on purpose.
*/
void elan_ifctl_set(uint32_t value);

/** \brief  Enable CLXB address windows (IFCTL bit 2, never bit 0). */
void elan_pvr2_enable(void);

/** \brief  Disable CLXB address windows (IFCTL = 0). */
void elan_pvr2_disable(void);

/** \brief  True if IFCTL bit 2 is set. */
bool elan_pvr2_enabled(void);

/** @} */

__END_DECLS

#endif /* __DC_ELAN_H */
