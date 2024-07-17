/* KallistiOS ##version##

   dc/pvr/pvr_prim.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2014 Lawrence Sebald
   Copyright (C) 2023 Ruslan Rostovtsev
   Copyright (C) 2024 Falco Girgis
*/

/** \file       dc/pvr/pvr_prim.h
    \brief      API for working with PowerVR 3D primitive types
    \ingroup    

    \author Megan Potter
    \author Roger Cattermole
    \author Paul Boese
    \author Brian Paul
    \author Lawrence Sebald
    \author Benoit Miller
    \author Ruslan Rostovtsev
    \author Falco Girgis
*/

#ifndef __DC_PVR_PVR_PRIM_H
#define __DC_PVR_PVR_PRIM_H

#include <stdint.h>

#include <sys/cdefs.h>
__BEGIN_DECLS


/** \defgroup pvr_geometry Geometry
    \brief                 PVR API for managing scene geometry
    \ingroup               pvr
*/

/** \defgroup pvr_primitives Primitives
    \brief                   Polygon and sprite management
    \ingroup                 pvr_geometry
*/

/** \defgroup pvr_primitives_headers Headers
    \brief                           Compiled headers for polygons and sprites
    \ingroup pvr_primitives

    @{
*/

/** \defgroup pvr_commands          TA Command Values
    \brief                          Command values for submitting data to the TA
    \ingroup                        pvr_primitives_headers

    These are are appropriate values for TA commands. Use whatever goes with the
    primitive type you're using.

    @{
*/
#define PVR_CMD_POLYHDR     0x80840000  /**< \brief PVR polygon header. */
#define PVR_CMD_VERTEX      0xe0000000  /**< \brief PVR vertex data */
#define PVR_CMD_VERTEX_EOL  0xf0000000  /**< \brief PVR vertex, end of strip */
#define PVR_CMD_USERCLIP    0x20000000  /**< \brief PVR user clipping area */
#define PVR_CMD_MODIFIER    0x80000000  /**< \brief PVR modifier volume */
#define PVR_CMD_SPRITE      0xA0000000  /**< \brief PVR sprite header */
/** @} */

/** \defgroup pvr_bitmasks          Constants and Masks
    \brief                          Polygon header constants and masks
    \ingroup                        pvr_primitives_headers

    Note that thanks to the arrangement of constants, this is mainly a matter of
    bit shifting to compile headers...

    @{
*/
#define PVR_TA_CMD_TYPE_SHIFT           24
#define PVR_TA_CMD_TYPE_MASK            (7 << PVR_TA_CMD_TYPE_SHIFT)

#define PVR_TA_CMD_USERCLIP_SHIFT       16
#define PVR_TA_CMD_USERCLIP_MASK        (3 << PVR_TA_CMD_USERCLIP_SHIFT)

#define PVR_TA_CMD_CLRFMT_SHIFT         4
#define PVR_TA_CMD_CLRFMT_MASK          (7 << PVR_TA_CMD_CLRFMT_SHIFT)

#define PVR_TA_CMD_SPECULAR_SHIFT       2
#define PVR_TA_CMD_SPECULAR_MASK        (1 << PVR_TA_CMD_SPECULAR_SHIFT)

#define PVR_TA_CMD_SHADE_SHIFT          1
#define PVR_TA_CMD_SHADE_MASK           (1 << PVR_TA_CMD_SHADE_SHIFT)

#define PVR_TA_CMD_UVFMT_SHIFT          0
#define PVR_TA_CMD_UVFMT_MASK           (1 << PVR_TA_CMD_UVFMT_SHIFT)

#define PVR_TA_CMD_MODIFIER_SHIFT       7
#define PVR_TA_CMD_MODIFIER_MASK        (1 <<  PVR_TA_CMD_MODIFIER_SHIFT)

#define PVR_TA_CMD_MODIFIERMODE_SHIFT   6
#define PVR_TA_CMD_MODIFIERMODE_MASK    (1 <<  PVR_TA_CMD_MODIFIERMODE_SHIFT)

#define PVR_TA_PM1_DEPTHCMP_SHIFT       29
#define PVR_TA_PM1_DEPTHCMP_MASK        (7 << PVR_TA_PM1_DEPTHCMP_SHIFT)

#define PVR_TA_PM1_CULLING_SHIFT        27
#define PVR_TA_PM1_CULLING_MASK         (3 << PVR_TA_PM1_CULLING_SHIFT)

#define PVR_TA_PM1_DEPTHWRITE_SHIFT     26
#define PVR_TA_PM1_DEPTHWRITE_MASK      (1 << PVR_TA_PM1_DEPTHWRITE_SHIFT)

#define PVR_TA_PM1_TXRENABLE_SHIFT      25
#define PVR_TA_PM1_TXRENABLE_MASK       (1 << PVR_TA_PM1_TXRENABLE_SHIFT)

#define PVR_TA_PM1_MODIFIERINST_SHIFT   29
#define PVR_TA_PM1_MODIFIERINST_MASK    (3 <<  PVR_TA_PM1_MODIFIERINST_SHIFT)

#define PVR_TA_PM2_SRCBLEND_SHIFT       29
#define PVR_TA_PM2_SRCBLEND_MASK        (7 << PVR_TA_PM2_SRCBLEND_SHIFT)

#define PVR_TA_PM2_DSTBLEND_SHIFT       26
#define PVR_TA_PM2_DSTBLEND_MASK        (7 << PVR_TA_PM2_DSTBLEND_SHIFT)

#define PVR_TA_PM2_SRCENABLE_SHIFT      25
#define PVR_TA_PM2_SRCENABLE_MASK       (1 << PVR_TA_PM2_SRCENABLE_SHIFT)

#define PVR_TA_PM2_DSTENABLE_SHIFT      24
#define PVR_TA_PM2_DSTENABLE_MASK       (1 << PVR_TA_PM2_DSTENABLE_SHIFT)

#define PVR_TA_PM2_FOG_SHIFT            22
#define PVR_TA_PM2_FOG_MASK             (3 << PVR_TA_PM2_FOG_SHIFT)

#define PVR_TA_PM2_CLAMP_SHIFT          21
#define PVR_TA_PM2_CLAMP_MASK           (1 << PVR_TA_PM2_CLAMP_SHIFT)

#define PVR_TA_PM2_ALPHA_SHIFT          20
#define PVR_TA_PM2_ALPHA_MASK           (1 << PVR_TA_PM2_ALPHA_SHIFT)

#define PVR_TA_PM2_TXRALPHA_SHIFT       19
#define PVR_TA_PM2_TXRALPHA_MASK        (1 << PVR_TA_PM2_TXRALPHA_SHIFT)

#define PVR_TA_PM2_UVFLIP_SHIFT         17
#define PVR_TA_PM2_UVFLIP_MASK          (3 << PVR_TA_PM2_UVFLIP_SHIFT)

#define PVR_TA_PM2_UVCLAMP_SHIFT        15
#define PVR_TA_PM2_UVCLAMP_MASK         (3 << PVR_TA_PM2_UVCLAMP_SHIFT)

#define PVR_TA_PM2_FILTER_SHIFT         12
#define PVR_TA_PM2_FILTER_MASK          (7 << PVR_TA_PM2_FILTER_SHIFT)

#define PVR_TA_PM2_MIPBIAS_SHIFT        8
#define PVR_TA_PM2_MIPBIAS_MASK         (15 << PVR_TA_PM2_MIPBIAS_SHIFT)

#define PVR_TA_PM2_TXRENV_SHIFT         6
#define PVR_TA_PM2_TXRENV_MASK          (3 << PVR_TA_PM2_TXRENV_SHIFT)

#define PVR_TA_PM2_USIZE_SHIFT          3
#define PVR_TA_PM2_USIZE_MASK           (7 << PVR_TA_PM2_USIZE_SHIFT)

#define PVR_TA_PM2_VSIZE_SHIFT          0
#define PVR_TA_PM2_VSIZE_MASK           (7 << PVR_TA_PM2_VSIZE_SHIFT)

#define PVR_TA_PM3_MIPMAP_SHIFT         31
#define PVR_TA_PM3_MIPMAP_MASK          (1 << PVR_TA_PM3_MIPMAP_SHIFT)

#define PVR_TA_PM3_TXRFMT_SHIFT         0
#define PVR_TA_PM3_TXRFMT_MASK          0xffffffff
/** @} */

/** PVR polygon header.

    This is the hardware equivalent of a rendering context; you'll create one of
    these from your pvr_poly_cxt_t and use it for submission to the hardware.
*/
typedef struct pvr_poly_hdr {
    uint32_t cmd;                /**< \brief TA command */
    uint32_t mode1;              /**< \brief Parameter word 1 */
    uint32_t mode2;              /**< \brief Parameter word 2 */
    uint32_t mode3;              /**< \brief Parameter word 3 */
    uint32_t d1;                 /**< \brief Dummy value */
    uint32_t d2;                 /**< \brief Dummy value */
    uint32_t d3;                 /**< \brief Dummy value */
    uint32_t d4;                 /**< \brief Dummy value */
} pvr_poly_hdr_t;

/** PVR polygon header with intensity color.

    This is the equivalent of pvr_poly_hdr_t, but for use with intensity color.
*/
typedef struct pvr_poly_ic_hdr {
    uint32_t cmd;                /**< \brief TA command */
    uint32_t mode1;              /**< \brief Parameter word 1 */
    uint32_t mode2;              /**< \brief Parameter word 2 */
    uint32_t mode3;              /**< \brief Parameter word 3 */
    float   a;                   /**< \brief Face color alpha component */
    float   r;                   /**< \brief Face color red component */
    float   g;                   /**< \brief Face color green component */
    float   b;                   /**< \brief Face color blue component */
} pvr_poly_ic_hdr_t;

/** PVR polygon header to be used with modifier volumes.

    This is the equivalent of a pvr_poly_hdr_t for use when a polygon is to be
    used with modifier volumes.
*/
typedef struct pvr_poly_mod_hdr {
    uint32_t cmd;                /**< \brief TA command */
    uint32_t mode1;              /**< \brief Parameter word 1 */
    uint32_t mode2_0;            /**< \brief Parameter word 2 (outside volume) */
    uint32_t mode3_0;            /**< \brief Parameter word 3 (outside volume) */
    uint32_t mode2_1;            /**< \brief Parameter word 2 (inside volume) */
    uint32_t mode3_1;            /**< \brief Parameter word 3 (inside volume) */
    uint32_t d1;                 /**< \brief Dummy value */
    uint32_t d2;                 /**< \brief Dummy value */
} pvr_poly_mod_hdr_t;

/** PVR polygon header specifically for sprites.

    This is the equivalent of a pvr_poly_hdr_t for use when a quad/sprite is to
    be rendered. Note that the color data is here, not in the vertices.
*/
typedef struct pvr_sprite_hdr {
    uint32_t cmd;                /**< \brief TA command */
    uint32_t mode1;              /**< \brief Parameter word 1 */
    uint32_t mode2;              /**< \brief Parameter word 2 */
    uint32_t mode3;              /**< \brief Parameter word 3 */
    uint32_t argb;               /**< \brief Sprite face color */
    uint32_t oargb;              /**< \brief Sprite offset color */
    uint32_t d1;                 /**< \brief Dummy value */
    uint32_t d2;                 /**< \brief Dummy value */
} pvr_sprite_hdr_t;

/** Modifier volume header.

    This is the header that should be submitted when dealing with setting a
    modifier volume.
*/
typedef struct pvr_mod_hdr {
    uint32_t cmd;                /**< \brief TA command */
    uint32_t mode1;              /**< \brief Parameter word 1 */
    uint32_t d1;                 /**< \brief Dummy value */
    uint32_t d2;                 /**< \brief Dummy value */
    uint32_t d3;                 /**< \brief Dummy value */
    uint32_t d4;                 /**< \brief Dummy value */
    uint32_t d5;                 /**< \brief Dummy value */
    uint32_t d6;                 /**< \brief Dummy value */
} pvr_mod_hdr_t;

/** @} */

/** \defgroup pvr_vertex_types  Vertices
    \brief                      PowerVR vertex types
    \ingroup                    pvr_geometry

    @{
*/

/** Generic PVR vertex type.

    The PVR chip itself supports many more vertex types, but this is the main
    one that can be used with both textured and non-textured polygons, and is
    fairly fast.

    \sa pvr_vertex_pcm, pvr_vertex_tpcm
*/
typedef struct pvr_vertex {
    uint32_t flags;              /**< \brief TA command (vertex flags) */
    float   x;                   /**< \brief X coordinate */
    float   y;                   /**< \brief Y coordinate */
    float   z;                   /**< \brief Z coordinate */
    float   u;                   /**< \brief Texture U coordinate */
    float   v;                   /**< \brief Texture V coordinate */
    uint32_t argb;               /**< \brief Vertex color */
    uint32_t oargb;              /**< \brief Vertex offset color */
} pvr_vertex_t;

/** PVR vertex type: Non-textured, packed color, modifiers.

    This vertex type has two copies of colors. The second color is used when
    enclosed within a modifier volume.

    \sa pvr_vertex, pvr_vertex_tpcm
*/
typedef struct pvr_vertex_pcm {
    uint32_t flags;              /**< \brief TA command (vertex flags) */
    float   x;                   /**< \brief X coordinate */
    float   y;                   /**< \brief Y coordinate */
    float   z;                   /**< \brief Z coordinate */
    uint32_t argb0;              /**< \brief Vertex color (outside volume) */
    uint32_t argb1;              /**< \brief Vertex color (inside volume) */
    uint32_t d1;                 /**< \brief Dummy value */
    uint32_t d2;                 /**< \brief Dummy value */
} pvr_vertex_pcm_t;

/** PVR vertex type: Textured, packed color, modifiers.

    Note that this vertex type has two copies of colors, offset colors, and
    texture coords. The second set of texture coords, colors, and offset colors
    are used when enclosed within a modifier volume.

    \sa pvr_vertex, pvr_vertex_pcm
*/
typedef struct pvr_vertex_tpcm {
    uint32_t flags;              /**< \brief TA command (vertex flags) */
    float   x;                  /**< \brief X coordinate */
    float   y;                  /**< \brief Y coordinate */
    float   z;                  /**< \brief Z coordinate */
    float   u0;                 /**< \brief Texture U coordinate (outside) */
    float   v0;                 /**< \brief Texture V coordinate (outside) */
    uint32_t argb0;              /**< \brief Vertex color (outside) */
    uint32_t oargb0;             /**< \brief Vertex offset color (outside) */
    float   u1;                 /**< \brief Texture U coordinate (inside) */
    float   v1;                 /**< \brief Texture V coordinate (inside) */
    uint32_t argb1;              /**< \brief Vertex color (inside) */
    uint32_t oargb1;             /**< \brief Vertex offset color (inside) */
    uint32_t d1;                 /**< \brief Dummy value */
    uint32_t d2;                 /**< \brief Dummy value */
    uint32_t d3;                 /**< \brief Dummy value */
    uint32_t d4;                 /**< \brief Dummy value */
} pvr_vertex_tpcm_t;

/** PVR vertex type: Textured sprite.

    This vertex type is to be used with the sprite polygon header and the sprite
    related commands to draw textured sprites. Note that there is no fourth Z
    coordinate. I suppose it just gets interpolated?

    The U/V coordinates in here are in the 16-bit per coordinate form. Also,
    like the fourth Z value, there is no fourth U or V, so it must get
    interpolated from the others.

    \sa pvr_sprite_col
*/
typedef struct pvr_sprite_txr {
    uint32_t flags;               /**< \brief TA command (vertex flags) */
    float   ax;                   /**< \brief First X coordinate */
    float   ay;                   /**< \brief First Y coordinate */
    float   az;                   /**< \brief First Z coordinate */
    float   bx;                   /**< \brief Second X coordinate */
    float   by;                   /**< \brief Second Y coordinate */
    float   bz;                   /**< \brief Second Z coordinate */
    float   cx;                   /**< \brief Third X coordinate */
    float   cy;                   /**< \brief Third Y coordinate */
    float   cz;                   /**< \brief Third Z coordinate */
    float   dx;                   /**< \brief Fourth X coordinate */
    float   dy;                   /**< \brief Fourth Y coordinate */
    uint32_t dummy;               /**< \brief Dummy value */
    uint32_t auv;                 /**< \brief First U/V texture coordinates */
    uint32_t buv;                 /**< \brief Second U/V texture coordinates */
    uint32_t cuv;                 /**< \brief Third U/V texture coordinates */
} pvr_sprite_txr_t;

/** PVR vertex type: Untextured sprite.

    This vertex type is to be used with the sprite polygon header and the sprite
    related commands to draw untextured sprites (aka, quads).

    \sa pvr_sprite_txr
*/
typedef struct pvr_sprite_col {
    uint32_t flags;              /**< \brief TA command (vertex flags) */
    float   ax;                  /**< \brief First X coordinate */
    float   ay;                  /**< \brief First Y coordinate */
    float   az;                  /**< \brief First Z coordinate */
    float   bx;                  /**< \brief Second X coordinate */
    float   by;                  /**< \brief Second Y coordinate */
    float   bz;                  /**< \brief Second Z coordinate */
    float   cx;                  /**< \brief Third X coordinate */
    float   cy;                  /**< \brief Third Y coordinate */
    float   cz;                  /**< \brief Third Z coordinate */
    float   dx;                  /**< \brief Fourth X coordinate */
    float   dy;                  /**< \brief Fourth Y coordinate */
    uint32_t d1;                 /**< \brief Dummy value */
    uint32_t d2;                 /**< \brief Dummy value */
    uint32_t d3;                 /**< \brief Dummy value */
    uint32_t d4;                 /**< \brief Dummy value */
} pvr_sprite_col_t;

/** PVR vertex type: Modifier volume.

    This vertex type is to be used with the modifier volume header to specify
    triangular modifier areas.
*/
typedef struct pvr_modifier_vol {
    uint32_t flags;              /**< \brief TA command (vertex flags) */
    float   ax;                  /**< \brief First X coordinate */
    float   ay;                  /**< \brief First Y coordinate */
    float   az;                  /**< \brief First Z coordinate */
    float   bx;                  /**< \brief Second X coordinate */
    float   by;                  /**< \brief Second Y coordinate */
    float   bz;                  /**< \brief Second Z coordinate */
    float   cx;                  /**< \brief Third X coordinate */
    float   cy;                  /**< \brief Third Y coordinate */
    float   cz;                  /**< \brief Third Z coordinate */
    uint32_t d1;                 /**< \brief Dummy value */
    uint32_t d2;                 /**< \brief Dummy value */
    uint32_t d3;                 /**< \brief Dummy value */
    uint32_t d4;                 /**< \brief Dummy value */
    uint32_t d5;                 /**< \brief Dummy value */
    uint32_t d6;                 /**< \brief Dummy value */
} pvr_modifier_vol_t;

/** @} */

__END_DECLS

#endif
