/* KallistiOS ##version##

    dc/minifont.h
    Copyright (C) 2020 Lawrence Sebald

*/

/** \file    dc/minifont.h
    \brief   Simple font drawing functions.
    \ingroup video_fonts_mini

    This file provides support for utilizing the "Naomi" font that is included
    in the KOS source code (in the utils/minifont.h file). This was designed for
    use when you really just want a *very* simple font to draw with.

    \note
    Only ASCII characters are usable here. No other fancy encodings are
    supported, nor are any extended ASCII characters beyond the 7-bit range.
    Also, only 16-bit buffers (like what you would normally have for the
    framebuffer) are currently supported.

    \author Lawrence Sebald
*/

#ifndef __DC_MINIFONT_H
#define __DC_MINIFONT_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/** \defgroup video_fonts_mini Mini
    \brief                     Extra mini-font provided for Dreamcast and NAOMI
    \ingroup                   video_fonts

    @{
*/

/** \brief  Draw a single character to a buffer.

    This function draws a single character to the given buffer.

    \param  buffer          The buffer to draw to (at least 8 x 16 pixels)
    \param  bufwidth        The width of the buffer in pixels
    \param  c               The character to draw

    \return                 Amount of width covered in 16-bit increments.
*/
int minifont_draw(uint16 *buffer, uint32 bufwidth, uint32 c);

/** \brief  Draw a full string to any sort of buffer.

    This function draws a NUL-terminated string to the given buffer. Only
    standard ASCII encoded strings are supported (no extended ASCII, ANSI,
    Unicode, JIS, EUC, etc).

    \param  b               The buffer to draw to.
    \param  bufwidth           The width of the buffer in pixels.
    \param  str             The string to draw.

    \return                 Amount of width covered in 16-bit increments.
*/
int minifont_draw_str(uint16 *b, uint32 bufwidth, const char *str);

/** @} */

__END_DECLS

#endif  /* __DC_MINIFONT_H */
