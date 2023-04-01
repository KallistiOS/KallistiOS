/* KallistiOS ##version##

   dc/vblank.h
   Copyright (C)2003 Megan Potter

*/

/** \file   dc/vblank.h
    \brief  VBlank handler registration.

    This file allows functions to be registered to be called on each vblank
    interrupt that occurs. This gives a way to schedule small functions that
    must occur regularly, without using threads.

    \author Megan Potter
*/

#ifndef __DC_VBLANK_H
#define __DC_VBLANK_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <dc/asic.h>

/** \brief  Add a vblank handler.

    This function adds a handler to the vblank handler list. The function will
    be called at the start of every vblank period with the same parameters that
    were passed to the IRQ handler for vblanks.

    \param  hnd             The handler to add.
    \return                 The handle id on success, or <0 on failure.
*/
int vblank_handler_add(asic_evt_handler hnd);

/** \brief  Remove a vblank handler.

    This function removes the specified handler from the vblank handler list.

    \param  handle          The handle id to remove (returned by
                            vblank_handler_add() when the handler was added).
    \retval 0               On success.
    \retval -1              On failure.
*/
int vblank_handler_remove(int handle);

/* \cond */
/** Initialize the vblank handler. This must be called after the asic module
    is initialized. */
int vblank_init();

/** Shut down the vblank handler. */
int vblank_shutdown();
/* \endcond */

__END_DECLS

#endif  /* __DC_VBLANK_H */

