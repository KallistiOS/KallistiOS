/* KallistiOS ##version##

   dc/dev_fb.h
   (c)2024 Donald Haase

*/

/** \file    dc/dev_fb.h
    \brief   /dev/fb driver.
    \ingroup dev_fb



    \author Megan Potter

    \see    dc/video.h
    \see    linux/fb.h
*/

#ifndef __DC_DEV_FB_H
#define __DC_DEV_FB_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/fs.h>

/** \defgroup dev_fb    /dev/fb
    \brief              VFS driver for accessing the framebuffer
    \ingroup            vfs

    @{
*/

/* \cond */
/* Initialization */
int fs_fb_init(void);
int fs_fb_shutdown(void);
/* \endcond */

/** @} */

__END_DECLS

#endif  /* __DC_DEV_FB_H */

