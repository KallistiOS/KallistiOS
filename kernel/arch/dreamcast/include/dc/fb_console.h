/* KallistiOS ##version##

   dc/fb_console.h
   Copyright (C) 2009 Lawrence Sebald

*/

/** \file    dc/fb_console.h
    \brief   A simple dbgio interface to draw to the framebuffer.
    \ingroup logging

    This file contains definitions to interact with a simple framebuffer dbgio
    interface. This interface can be moved around in memory and can have its
    width and height set so that you can truly customize it to the environment
    as needed. This utilizes the bios font functionality to actually draw any
    characters.

    To actually use the framebuffer device, pass "fb" as the parameter to
    dbgio_dev_select().

    \author Lawrence Sebald
*/

#ifndef __DC_FB_CONSOLE_H
#define __DC_FB_CONSOLE_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <kos/dbgio.h>

/* \cond */
extern dbgio_handler_t dbgio_fb;
/* \endcond */

/** \brief  Set the target for the framebuffer dbgio device.

    This function allows you to set a target for the dbgio device on the
    framebuffer. This allows you to do things like setting it to render to a
    texture rather than to the whole framebuffer, for instance.

    The default setup for the framebuffer dbgio device is to print to the full
    640x480 framebuffer (minus a 32-pixel border around the outside). If you
    change this, you can restore the original functionality by passing NULL for
    t, 640 for w, 480 for h, and 32 for borderx and bordery.

    \param  t               The target in memory to render to.
    \param  w               The width of the target.
    \param  h               The height of the target.
    \param  borderx         How much border to leave around the target in the
                            X direction.
    \param  bordery         How much border to leave around the target in the
                            Y direction.
*/
void dbgio_fb_set_target(uint16 *t, int w, int h, int borderx, int bordery);

__END_DECLS

#endif /* __DC_FB_CONSOLE_H */
