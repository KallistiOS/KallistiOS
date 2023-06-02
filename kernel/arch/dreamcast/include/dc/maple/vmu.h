/* KallistiOS ##version##

   dc/maple/vmu.h
   Copyright (C)2000-2002 Jordan DeLong and Megan Potter
   Copyright (C)2008 Donald Haase

*/

/** \file   dc/maple/vmu.h
    \brief  Definitions for using the VMU device.

    This file contains the definitions needed to access the Maple VMU device.
    This includes all of the functionality of memory cards, including the
    MAPLE_FUNC_MEMCARD, MAPLE_FUNC_LCD, and MAPLE_FUNC_CLOCK function codes.

    \author Jordan DeLong
    \author Megan Potter
    \author Donald Haase
*/

#ifndef __DC_MAPLE_VMU_H
#define __DC_MAPLE_VMU_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <dc/maple.h>

/** \brief  Enable custom color of a VMU

    This function enables/disables the custom color of a specific VMU. 
    This color is only displayed in the Dreamcast's file manager.

    \param  dev             The device to enable custom color.
    \param  enable          Values other than 0 enables. Equal to 0 disables.
    \retval 0               On success
    \retval -1              On failure
*/
int vmu_use_custom_color(maple_device_t * dev, int enable);

/** \brief  Set custom color of a VMU

    This function sets the custom color of a specific VMU. This color is only
    displayed in the Dreamcast's file manager. This function also enables the 
    use of the custom color. Otherwise it wouldn't show up.

    \param  dev             The device to change the color of.
    \param  red             The red component. 0-255
    \param  green           The green component. 0-255
    \param  blue            The blue component. 0-255
    \param  alpha           The alpha component. 0-255; 100-255 Recommended
    \retval 0               On success
    \retval -1              On failure
*/
int vmu_set_custom_color(maple_device_t * dev, uint8 red, uint8 green, uint8 blue, uint8 alpha);

/** \brief  Set icon shape of a VMU

    This function sets the icon shape of a specific VMU. The icon shape is a 
    vmu icon that is displayed on the LCD screen while navigating the Dreamcast
    BIOS menu and is the GUI representation of the VMU in the menu's file manager. 
    The Dreamcast BIOS provides a set of 124 icons to choose from. The set of icons 
    you can choose from are located in biosfont.h and start with BFONT_VMUICON and 
    end with BFONT_EMBROIDERY.

    \param  dev             The device to change the icon shape of.
    \param  icon_shape      The valid values for icon_shape are BFONT_* listed in 
                            the biosfont.h
    \retval 0               On success
    \retval -1              On failure
*/
int vmu_set_icon_shape(maple_device_t * dev, uint8 icon_shape);

/** \brief  Make a VMU beep.

    This function sends a raw beep to a VMU, causing the speaker to emit a tone
    noise. See http://dcemulation.org/phpBB/viewtopic.php?f=29&t=97048 for the
    original information about beeping.

    \param  dev             The device to attempt to beep.
    \param  beep            The tone to generate. Actual parameters unknown.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EAGAIN    If the command couldn't be sent. Try again later.
    \retval MAPLE_ETIMEOUT  If the command timed out while blocking.
*/
int vmu_beep_raw(maple_device_t * dev, uint32 beep);

/** \brief  Display a 1bpp bitmap on a VMU screen.

    This function sends a raw bitmap to a VMU to display on its screen. This
    bitmap is 1bpp, and is 48x32 in size.

    \param  dev             The device to draw to.
    \param  bitmap          The bitmap to show.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EAGAIN    If the command couldn't be sent. Try again later.
    \retval MAPLE_ETIMEOUT  If the command timed out while blocking.
*/
int vmu_draw_lcd(maple_device_t * dev, const void *bitmap);

/** \brief  Display a 1bpp bitmap on a VMU screen.

    This function sends a raw bitmap to a VMU to display on its screen. This
    bitmap is 1bpp, and is 48x32 in size. This function is equivalent to
    vmu_draw_lcd, but the image is rotated 180° so that the first byte of the
    bitmap corresponds to the top-left corner, instead of the bottom-right one.

    \param  dev             The device to draw to.
    \param  bitmap          The bitmap to show.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EAGAIN    If the command couldn't be sent. Try again later.
    \retval MAPLE_ETIMEOUT  If the command timed out while blocking.

    \warning    This function is optimized by an assembly routine which operates
                on 32 bits at a time. As such, the given bitmap must be 4-byte
		aligned.
*/
int vmu_draw_lcd_rotated(maple_device_t * dev, const void *bitmap);

/** \brief  Display a Xwindows XBM image on a VMU screen.

    This function takes in a Xwindows XBM, converts it to a raw bitmap, and sends 
    it to a VMU to display on its screen. This XBM image is 48x32 in size.

    \param  dev             The device to draw to.
    \param  vmu_icon        The icon to set.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EAGAIN    If the command couldn't be sent. Try again later.
    \retval MAPLE_ETIMEOUT  If the command timed out while blocking.
*/
int vmu_draw_lcd_xbm(maple_device_t * dev, const char *vmu_icon);

/** \brief  Read a block from a memory card.

    This function reads a raw block from a memory card. You most likely will not
    ever use this directly, but rather will probably use the fs_vmu stuff.

    \param  dev             The device to read from.
    \param  blocknum        The block number to read.
    \param  buffer          The buffer to read into (512 bytes).
    \retval MAPLE_EOK       On success.
    \retval MAPLE_ETIMEOUT  If the command timed out while blocking.
    \retval MAPLE_EFAIL     On errors other than timeout.
*/
int vmu_block_read(maple_device_t * dev, uint16 blocknum, uint8 *buffer);

/** \brief  Write a block to a memory card.

    This function writes a raw block to a memory card. You most likely will not
    ever use this directly, but rather will probably use the fs_vmu stuff.

    \param  dev             The device to write to.
    \param  blocknum        The block number to write.
    \param  buffer          The buffer to write from (512 bytes).
    \retval MAPLE_EOK       On success.
    \retval MAPLE_ETIMEOUT  If the command timed out while blocking.
    \retval MAPLE_EFAIL     On errors other than timeout.
*/
int vmu_block_write(maple_device_t * dev, uint16 blocknum, uint8 *buffer);

/** \brief  Set a Xwindows XBM on all VMUs.

    This function takes in a Xwindows XBM and sets the image on all VMUs. This
    is a convenience function for vmu_draw_lcd() to broadcast across all VMUs.

    \param  vmu_icon        The icon to set.
*/
void vmu_set_icon(const char *vmu_icon);

/* \cond */
/* Init / Shutdown */
int vmu_init(void);
void vmu_shutdown(void);
/* \endcond */

__END_DECLS

#endif  /* __DC_MAPLE_VMU_H */

