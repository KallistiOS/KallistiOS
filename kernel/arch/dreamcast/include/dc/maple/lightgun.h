/* KallistiOS ##version##

   dc/maple/lightgun.h
   Copyright (C) 2015 Lawrence Sebald

*/

/** \file    dc/maple/lightgun.h
    \brief   Definitions for using the light gun.
    \ingroup peripherals_lightgun

    This file contains the definitions needed to access maple light gun devices.
    There's really no user-serviceable parts in here, as there's very little to
    this driver.

    \author Lawrence Sebald
*/

#ifndef __DC_MAPLE_LIGHTGUN_H
#define __DC_MAPLE_LIGHTGUN_H

#include <kos/cdefs.h>
__BEGIN_DECLS

/** \defgroup peripherals_lightgun  Lightgun
    \brief                          Maple driver for the lightgun accessory
    \ingroup                        peripherals

    @{
*/

/* \cond */
/* Init / Shutdown */
void lightgun_init(void);
void lightgun_shutdown(void);
/* \endcond */

/** @} */

__END_DECLS

#endif  /* __DC_MAPLE_LIGHTGUN_H */
