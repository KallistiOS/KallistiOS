/* KallistiOS ##version##

   dc/vblank.h
   Copyright (C) 2003 Megan Potter

*/

/** \file    dc/vblank.h
    \brief   VBlank handler registration.
    \ingroup system_vblank

    This file allows functions to be registered to be called on each vblank
    interrupt that occurs. This gives a way to schedule small functions that
    must occur regularly, without using threads.

    \author Megan Potter
*/

#ifndef __DC_VBLANK_H
#define __DC_VBLANK_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <dc/asic.h>

/** \defgroup system_vblank     VBlank
    \brief                      VBlank interrupt handler management
    \ingroup                    system

    @{
*/

struct vblhnd;

/** \struct  vblhnd_t
    \brief   Opaque structure describing one vblank handler.
*/
typedef struct vblhnd vblhnd_t;

/** \brief  Add a vblank handler.

    This function adds a handler to the vblank handler list. The function will
    be called at the start of every vblank period with the same parameters that
    were passed to the IRQ handler for vblanks.

    \param  hnd             The handler to add.
    \param  data            A user pointer that will be passed to the callback.

    \return                 The handle on success, or NULL on failure.
*/
vblhnd_t *vblank_handler_add(asic_evt_handler hnd, void *data);

/** \brief  Remove a vblank handler.

    This function removes the specified handler from the vblank handler list.

    \param  handle          The handle to remove (returned by
                            vblank_handler_add() when the handler was added).
*/
void vblank_handler_remove(vblhnd_t *handle);

/* \cond */
/** Initialize the vblank handler. This must be called after the asic module
    is initialized. */
int vblank_init(void);

/** Shut down the vblank handler. */
int vblank_shutdown(void);
/* \endcond */

/** @} */

__END_DECLS

#endif  /* __DC_VBLANK_H */

