/* KallistiOS ##version##

   dc/maple/mouse.h
   (C)2000-2002 Jordan DeLong and Megan Potter

*/

/** \file    dc/maple/mouse.h
    \brief   Definitions for using the mouse device.
    \ingroup mouse

    This file contains the definitions needed to access the Maple mouse type
    device.

    \author Jordan DeLong
    \author Megan Potter
*/

#ifndef __DC_MAPLE_MOUSE_H
#define __DC_MAPLE_MOUSE_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <kos/regfield.h>

/** \defgroup   mouse   Mouse
    \brief              Driver for the Dreamcast's Mouse Input Device
    \ingroup            peripherals
*/

/** \defgroup   mouse_buttons   Buttons
    \brief                      Masks for the buttons on a mouse
    \ingroup                    mouse

    These are the possible buttons to press on a maple bus mouse.

    @{
*/
#define MOUSE_RIGHTBUTTON   BIT(1)  /**< \brief Right mouse button */
#define MOUSE_LEFTBUTTON    BIT(2)  /**< \brief Left mouse button */
#define MOUSE_SIDEBUTTON    BIT(3)  /**< \brief Side mouse button */
/** @} */

/** \brief   Mouse center value in the raw condition structure. 
    \ingroup mouse
 */
#define MOUSE_DELTA_CENTER      0x200

/** \cond */
typedef struct {
    uint16_t    buttons;
    uint16_t    dummy1;
    int16_t     dx;
    int16_t     dy;
    int16_t     dz;
    uint16_t    dummy2;
    uint32_t    dummy3;
    uint32_t    dummy4;
} mouse_cond_t;
/** \endcond */

/* More civilized mouse structure. There are several significant
   differences in data interpretation between the "cooked" and
   the old "raw" structs:

   - buttons are zero-based: a 1-bit means the button is PRESSED
   - no dummy values

   Note that this is what maple_dev_status() will return.
 */

/** \brief   Mouse status structure.
    \ingroup mouse

    This structure contains information about the status of the mouse device,
    and can be fetched with maple_dev_status().

    \headerfile dc/maple/mouse.h
*/
typedef struct {
    /** \brief  Buttons pressed bitmask.
        \see    mouse_buttons
    */
    uint32_t  buttons;

    /** \brief  X movement value */
    int dx;

    /** \brief  Y movement value */
    int dy;

    /** \brief  Z movement value */
    int dz;
} mouse_state_t;

/* \cond */
/* Init / Shutdown */
void mouse_init(void);
void mouse_shutdown(void);
/* \endcond */

__END_DECLS

#endif  /* __DC_MAPLE_MOUSE_H */

