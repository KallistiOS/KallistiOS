/* KallistiOS ##version##

   dc/pvr/pvr_pal.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2014 Lawrence Sebald
   Copyright (C) 2023 Ruslan Rostovtsev
   Copyright (C) 2024 Falco Girgis
*/

/** \file       dc/pvr/pvr_pal.h
    \brief      Palette API for the PowerVR
    \ingroup    pvr_pal_mgmt

    \author Megan Potter
    \author Roger Cattermole
    \author Paul Boese
    \author Brian Paul
    \author Lawrence Sebald
    \author Benoit Miller
    \author Ruslan Rostovtsev
    \author Falco Girgis
*/

#ifndef __DC_PVR_PVR_PALETTE_H
#define __DC_PVR_PVR_PALETTE_H

#include <stdint.h>

#include <kos/cdefs.h>
__BEGIN_DECLS

/** \defgroup pvr_pal_mgmt  Palettes
    \brief                  Color palette management API of the PowerVR
    \ingroup                pvr_global

    In addition to its 16-bit truecolor modes, the PVR also supports some
    nice paletted modes. 

    \remark
    These aren't useful for super high quality images most of the time,
    but they can be useful for doing some interesting special effects,
    like the old cheap "worm hole". 
*/

/** \brief   Color palette formats of the PowerVR
    \ingroup pvr_pal_mgmt

    Entries in the PVR's palettes can be of any of these formats. Note that you
    can only have one format active at a time.
*/
typedef enum pvr_palfmt {
    PVR_PAL_ARGB1555, /**< 16-bit ARGB1555 palette format */
    PVR_PAL_RGB565,   /**< 16-bit RGB565 palette format */
    PVR_PAL_ARGB4444, /**< 16-bit ARGB4444 palette format */
    PVR_PAL_ARGB8888  /**< 32-bit ARGB8888 palette format */
} pvr_palfmt_t;

/** \brief   Set the palette format.
    \ingroup pvr_pal_mgmt

    This function sets the currently active palette format on the PVR. Each
    entry in the palette table is 32-bits in length, regardless of what color
    format is in use.

    \warning
    Be sure to use care when using the PVR_PAL_ARGB8888 format. Rendering speed
    is greatly affected (cut about in half) if you use any filtering with
    paletted textures with ARGB8888 entries in the palette.

    \param  fmt             The format to use
*/
void pvr_set_pal_format(pvr_palfmt_t fmt);

/** \brief   Set a palette value.
    \ingroup pvr_pal_mgmt

    Note that while the color format is variable, each entry is still 32-bits in
    length regardless (and you only get a total of 1024 of them). If using one
    of the 16-bit palette formats, only the low-order 16-bits of the entry are
    valid, and the high bits should be filled in with 0.

    \param  idx             The index to set to (0-1023)
    \param  value           The color value to set in that palette entry
*/
static inline void pvr_set_pal_entry(uint32_t idx, uint32_t value) {
    PVR_SET(PVR_PALETTE_TABLE_BASE + 4 * idx, value);
}

__END_DECLS 

#endif  /* __DC_PVR_PVR_PALETTE_H */
