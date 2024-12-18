/* KallistiOS ##version##

   pvr_palette.c
   (C)2002 Megan Potter

 */

#include <assert.h>
#include <dc/pvr.h>
#include "pvr_internal.h"

/*
   In addition to its 16-bit truecolor modes, the PVR also supports some
   nice paletted modes. These aren't useful for super high quality images
   most of the time, but they can be useful for doing some interesting
   special effects, like the old cheap "worm hole".
*/

/* Set the palette format */
void pvr_set_pal_format(pvr_palfmt_t fmt) {
    PVR_SET(PVR_PALETTE_CFG, fmt);
}

