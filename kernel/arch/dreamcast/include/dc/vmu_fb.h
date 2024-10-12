/* KallistiOS ##version##

   dc/vmu_fb.h
   Copyright (C) 2023 Paul Cercueil

*/

#ifndef __DC_VMU_FB_H
#define __DC_VMU_FB_H

/** \file    dc/vmu_fb.h
    \brief   VMU framebuffer.
    \ingroup vmu_fb

    This file provides an API that can be used to compose a 48x32 image that can
    then be displayed on the VMUs connected to the system.

    \author Paul Cercueil
*/

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <dc/maple.h>
#include <dc/maple/vmu.h>
#include <stdint.h>
#include <stdarg.h>

/** \defgroup vmu_fb Framebuffer
 *  \ingroup  vmu
 *
 *  This API provides a virtual framebuffer abstraction for the VMU with a
 *  series of convenient methods for drawing complex and dynamic content.
 *
 *  @{
*/

/** \brief Virtual framebuffer for the VMU

    This object contains a 48x32 monochrome framebuffer. It can be painted to,
    or displayed on one the VMUs connected to the system, using the API below.
 */
typedef struct vmufb {
    uint32_t data[VMU_SCREEN_WIDTH]; /**< Private framebuffer pixel data */
} vmufb_t;

/** \brief  VMU framebuffer font meta-data.

    This structure describes a font, including character sizes,
    layout, and a pointer to the raw font data.
 */
typedef struct vmufb_font {
    unsigned int id;        /**< Font id */
    unsigned int w;         /**< Character width in pixels */
    unsigned int h;         /**< Character height in pixels */
    size_t       stride;    /**< Size of one character in bytes */
    const char  *fontdata;  /**< Pointer to the font data */
} vmufb_font_t;

/** \brief  Render into the VMU framebuffer

    This function will paint the provided pixel data into the VMU framebuffer,
    into the rectangle provided by the x, y, w and h values.

    \param  fb              A pointer to the vmufb_t to paint to.
    \param  x               The horizontal position of the top-left corner of
                            the drawing area, in pixels
    \param  y               The vertical position of the top-left corner of the
                            drawing area, in pixels
    \param  w               The width of the drawing area, in pixels
    \param  h               The height of the drawing area, in pixels
    \param  data            A pointer to the pixel data that will be painted
                            into the drawing area.
 */
void vmufb_paint_area(vmufb_t *fb,
                      unsigned int x, unsigned int y,
                      unsigned int w, unsigned int h,
                      const char *data);

/** \brief  Clear a specific area of the VMU framebuffer

    This function clears the area of the VMU framebuffer designated by the
    x, y, w and h values.

    \param  fb              A pointer to the vmufb_t to paint to.
    \param  x               The horizontal position of the top-left corner of
                            the drawing area, in pixels
    \param  y               The vertical position of the top-left corner of the
                            drawing area, in pixels
    \param  w               The width of the drawing area, in pixels
    \param  h               The height of the drawing area, in pixels
 */
void vmufb_clear_area(vmufb_t *fb,
                      unsigned int x, unsigned int y,
                      unsigned int w, unsigned int h);

/** \brief  Clear the VMU framebuffer

    This function clears the whole VMU framebuffer.

    \param  fb              A pointer to the vmufb_t to paint to.
 */
void vmufb_clear(vmufb_t *fb);

/** \brief  Present the VMU framebuffer to a VMU

    This function presents the previously rendered VMU framebuffer to the
    VMU identified by the dev argument.

    \param  fb              A pointer to the vmufb_t to paint to.
    \param  dev             The maple device of the VMU to present to
 */
void vmufb_present(const vmufb_t *fb, maple_device_t *dev);

/** \brief  Render a string into the VMU framebuffer

    This function uses the provided font to render text into the VMU
    framebuffer.

    \param  fb              A pointer to the vmufb_t to paint to.
    \param  font            A pointer to the vmufb_font_t that will be used for
                            painting the text (or NULL to use the default)
    \param  x               The horizontal position of the top-left corner of
                            the drawing area, in pixels
    \param  y               The vertical position of the top-left corner of the
                            drawing area, in pixels
    \param  w               The width of the drawing area, in pixels
    \param  h               The height of the drawing area, in pixels
    \param  line_spacing    Specify the number of empty lines that should
                            separate two lines of text
    \param  str             The text to render
 */
void vmufb_print_string_into(vmufb_t *fb,
                             const vmufb_font_t *font,
                             unsigned int x, unsigned int y,
                             unsigned int w, unsigned int h,
                             unsigned int line_spacing,
                             const char *str);

/** \brief  Render a string into the VMU framebuffer

    Simplified version of vmufb_print_string_into(). This is the same as calling
    vmufb_print_string_into with x=0, y=0, w=48, h=32, line_spacing=0.

    \param  fb              A pointer to the vmufb_t to paint to.
    \param  font            A pointer to the vmufb_font_t that will be used for
                            painting the text (or NULL to use the default)
    \param  str             The text to render
 */
static __inline__
void vmufb_print_string(vmufb_t *fb, const vmufb_font_t *font,
                        const char *str) {
    vmufb_print_string_into(fb, font, 0, 0,
                            VMU_SCREEN_WIDTH, VMU_SCREEN_HEIGHT, 0, str);
}

/** \brief  Render a string to attached VMUs using the built-in font

    Uses the built-in VMU font to render a string to all VMUs connected to the
    system.

    \note
    The font currently set as the default font will be used.

    \param  fmt             The format string, optionally followed by extra
                            arguments.

    \sa vmu_set_font()
 */
void vmu_printf(const char *fmt, ...) __printflike(1, 2);

/** \brief Sets the default font for drawing text to the VMU.
 
    This function allows you to set a custom font for drawing text
    to the VMU screen. If the \p font parameter is set to `NULL`,
    the built-in VMU font will be used as the default.

    \warning
    The API does not take ownership of or copy \p font, so
    the given pointer must remain valid as long as it is set
    as the default!

    \param  font    Pointer to the font to set as default
    \returns        Pointer to the previous default font

    \sa vmu_get_font()
 */
const vmufb_font_t *vmu_set_font(const vmufb_font_t *font);

/** \brief Returns the default font used to draw text to the VMU.

    \returns    Pointer to the font currently set as the default

    \sa vmu_set_font()
 */
const vmufb_font_t *vmu_get_font(void);

/** @} */

__END_DECLS

#endif /* __DC_VMU_FB_H */
