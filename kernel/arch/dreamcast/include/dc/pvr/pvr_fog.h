/* KallistiOS ##version##

   dc/pvr/pvr_fog.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2014 Lawrence Sebald
   Copyright (C) 2023 Ruslan Rostovtsev
   Copyright (C) 2024 Falco Girgis
*/

/** \file       dc/pvr/pvr_fog.h
    \brief      Public API for the PVR's hardware fog
    \ingroup    pvr_fog

    \author Megan Potter
    \author Roger Cattermole
    \author Paul Boese
    \author Brian Paul
    \author Lawrence Sebald
    \author Benoit Miller
    \author Falco Girgis
*/

#ifndef __DC_PVR_PVR_FOG_H
#define __DC_PVR_PVR_FOG_H

#include <kos/cdefs.h>
__BEGIN_DECLS

/** \defgroup   pvr_fog     Fog
    \brief                  Hardware Fog API for the PowerVR
    \ingroup                pvr_global

    \todo Explain fog modes + equations

    \note 
    Thanks to Paul Boese for figuring this stuff out

    @{
*/

/** Set the table fog color.

    This function sets the color of fog for table fog. `0-1` range for all  colors.

    \param  a               Alpha value of the fog
    \param  r               Red value of the fog
    \param  g               Green value of the fog
    \param  b               Blue value of the fog
*/
void pvr_fog_table_color(float a, float r, float g, float b);

/** Set the vertex fog color.

    This function sets the fog color for vertex fog. `0-1` range for all colors.
    
    \todo
    This function is currently not implemented, as vertex fog is not supported
    by KOS. 
    
    \warning
    Calling this function will cause an assertion failure.

    \param  a               Alpha value of the fog
    \param  r               Red value of the fog
    \param  g               Green value of the fog
    \param  b               Blue value of the fog
*/
void pvr_fog_vertex_color(float a, float r, float g, float b);

/** Set the fog far depth.

    This function sets the `PVR_FOG_DENSITY` register appropriately for the
    specified value.

    \param  d               The depth to set
*/
void pvr_fog_far_depth(float d);

/** Initialize the fog table using an exp2 algorithm (like `GL_EXP2`).

    This function will automatically set the `PVR_FOG_DENSITY` register to
    `259.999999` as a part of its processing, then set up the fog table.

    \param  density         Fog density value

    \sa pvr_fog_table_exp(), pvr_fog_table_linear(), pvr_fog_table_custom()
*/
void pvr_fog_table_exp2(float density);

/** Initialize the fog table using an exp algorithm (like `GL_EXP`).

    This function will automatically set the `PVR_FOG_DENSITY` register to
    `259.999999` as a part of its processing, then set up the fog table.

    \param  density         Fog density value

    \sa pvr_fog_table_exp2(), pvr_fog_table_linear(), pvr_fog_table_custom()
*/
void pvr_fog_table_exp(float density);

/** Initialize the fog table using a linear algorithm (like `GL_LINEAR`).

    This function will set the `PVR_FOG_DENSITY` register to the as appropriate
    for the end value, and initialize the fog table for perspectively correct
    linear fog.

    \param  start           Fog start point
    \param  end             Fog end point

    \sa pvr_fog_table_exp(), pvr_fog_table_exp2(), pvr_fog_table_custom()
*/
void pvr_fog_table_linear(float start, float end);

/** Set a custom fog table from float values

    This function allows you to specify whatever values you need to for your fog
    parameters. All values should be clamped between `0` and `1`, and its your
    responsibility to set up the `PVR_FOG_DENSITY` register by calling
    pvr_fog_far_depth() with an appropriate value. The table passed in should
    have `129` entries, where the `0`th entry is farthest from the eye and the last
    entry is nearest. Higher values = heavier fog.

    \param  table            The table of fog values to set

    \sa pvr_fog_table_exp2(), pvr_fog_table_exp(), pvr_fog_table_linear()
*/
void pvr_fog_table_custom(float *table);

/** @} */

__END_DECLS

#endif  /* __DC_PVR_PVR_FOG_H */
