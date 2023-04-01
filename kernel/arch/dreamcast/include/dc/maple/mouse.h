/* KallistiOS ##version##

   dc/maple/mouse.h
   (C)2000-2002 Jordan DeLong and Megan Potter

*/

/** \file   dc/maple/mouse.h
    \brief  Definitions for using the mouse device.

    This file contains the definitions needed to access the Maple mouse type
    device.

    \author Jordan DeLong
    \author Megan Potter
*/

#ifndef __DC_MAPLE_MOUSE_H
#define __DC_MAPLE_MOUSE_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/** \defgroup   mouse_buttons   Mouse button codes

    These are the possible buttons to press on a maple bus mouse.

    @{
*/
#define MOUSE_RIGHTBUTTON   (1<<1)  /**< \brief Right mouse button */
#define MOUSE_LEFTBUTTON    (1<<2)  /**< \brief Left mouse button */
#define MOUSE_SIDEBUTTON    (1<<3)  /**< \brief Side mouse button */
/** @} */

/** \brief  Mouse center value in the raw condition structure. */
#define MOUSE_DELTA_CENTER      0x200

/* Raw mouse condition structure */
typedef struct {
    uint16 buttons;
    uint16 dummy1;
    int16 dx;
    int16 dy;
    int16 dz;
    uint16 dummy2;
    uint32 dummy3;
    uint32 dummy4;
} mouse_cond_t;

/* More civilized mouse structure. There are several significant
   differences in data interpretation between the "cooked" and
   the old "raw" structs:

   - buttons are zero-based: a 1-bit means the button is PRESSED
   - no dummy values

   Note that this is what maple_dev_status() will return.
 */

/** \brief  Mouse status structure.

    This structure contains information about the status of the mouse device,
    and can be fetched with maple_dev_status().

    \headerfile dc/maple/mouse.h
*/
typedef struct {
    /** \brief  Buttons pressed bitmask.
        \see    mouse_buttons
    */
    uint32  buttons;

    /** \brief  X movement value */
    int dx;

    /** \brief  Y movement value */
    int dy;

    /** \brief  Z movement value */
    int dz;
} mouse_state_t;

/* \cond */
/* Init / Shutdown */
int mouse_init();
void    mouse_shutdown();
/* \endcond */

__END_DECLS

#endif  /* __DC_MAPLE_MOUSE_H */

