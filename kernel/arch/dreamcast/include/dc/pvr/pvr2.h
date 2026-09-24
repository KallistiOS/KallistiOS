/* KallistiOS ##version##

   dc/pvr/pvr2.h
   Copyright (C) 2026 Ruslan Rostovtsev
*/

/** \file       dc/pvr/pvr2.h
    \brief      Second PowerVR (CLXB) on NAOMI 2
    \ingroup    pvr2

    Second PowerVR (CLXB) on NAOMI 2. Same registers and VRAM as CLXA,
    at \ref PVR2_ADDR_STRIDE. Those addresses do not decode until Elan
    IFCTL bit 2 is set (cleared at boot). \ref pvr2_init() sets that bit
    and initializes the memory controller so \ref PVR2_RAM_BASE can be
    used. Display output on this chip stays off.

    \author Ruslan Rostovtsev
*/

#ifndef __DC_PVR_PVR2_H
#define __DC_PVR_PVR2_H

#include <kos/cdefs.h>
__BEGIN_DECLS

/** \defgroup pvr2  Second PVR (NAOMI 2)
    \brief          CLXB bring-up and VRAM mapping
    \ingroup        pvr

    @{
*/

/** \brief   Enable CLXB windows and initialize its memory controller.

    \retval 0               On success (or already initialized)
    \retval -1              If Elan is not present
*/
int pvr2_init(void);

/** \brief   Reset CLXB and disable its address windows.

    \retval 0               On success
    \retval -1              If it was not initialized
*/
int pvr2_shutdown(void);

/** \brief   True if \ref pvr2_init() has succeeded in this process. */
int pvr2_enabled(void);

/** @} */

__END_DECLS

#endif /* __DC_PVR_PVR2_H */
