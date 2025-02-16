/* KallistiOS ##version##

   dc/pvr.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2014 Lawrence Sebald
   Copyright (C) 2023 Ruslan Rostovtsev

   Low-level PVR 3D interface for the DC
*/

/** \file    dc/pvr.h
    \brief   Low-level PVR (3D hardware) interface.
    \ingroup pvr

    This file provides support for using the PVR 3D hardware in the Dreamcast.
    Note that this does not handle any sort of perspective transformations or
    anything of the like. This is just a very thin wrapper around the actual
    hardware support.

    This file is used for pretty much everything related to the PVR, from memory
    management to actual primitive rendering.

    \note
    This API does \a not handle any sort of transformations
    (including perspective!) so for that, you should look to KGL.

    \author Megan Potter
    \author Roger Cattermole
    \author Paul Boese
    \author Brian Paul
    \author Lawrence Sebald
    \author Benoit Miller
    \author Ruslan Rostovtsev
*/

#ifndef __DC_PVR_H
#define __DC_PVR_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdalign.h>
#include <stdbool.h>

#include <arch/memory.h>
#include <arch/types.h>
#include <arch/cache.h>
#include <dc/sq.h>
#include <kos/img.h>
#include <kos/regfield.h>

/*  Note: This file also #includes headers from dc/pvr/. They are mostly
    at the bottom of the file to be able to use types defined throughout. */

#include "pvr/pvr_mem.h"

/** \defgroup pvr   PowerVR API
    \brief          Low-level PowerVR GPU Driver.
    \ingroup        video
*/

/* Data types ********************************************************/

/** \defgroup pvr_lists Polygon Lists
    \brief              Types pertaining to PVR list types: opaque, pt, tr, etc
    \ingroup            pvr
*/

/** \brief   PVR list specification.
    \ingroup pvr_lists

    Each primitive in the PVR is submitted to one of the hardware primitive
    lists. This type is an identifier for a list.

    \see    pvr_lists
*/
typedef uint32_t pvr_list_t;

/** \defgroup pvr_geometry Geometry
    \brief                 PVR API for managing scene geometry
    \ingroup               pvr
*/

/** \defgroup pvr_primitives Primitives
    \brief                   Polygon and sprite management
    \ingroup                 pvr_geometry
*/

/** \defgroup pvr_ctx Contexts
    \brief            User-friendly intermittent primitive representation
    \ingroup          pvr_primitives
*/

/** \brief   PVR polygon context.
    \ingroup pvr_ctx

    You should use this more human readable format for specifying your polygon
    contexts, and then compile them into polygon headers when you are ready to
    start using them.

    This has embedded structures in it for two reasons; the first reason is to
    make it easier for me to add new stuff later without breaking existing code.
    The second reason is to make it more readable and usable.

    Unfortunately, it seems that Doxygen chokes up a little bit on this
    structure, and others like it. The documentation should still be mostly
    understandable though...

    \headerfile dc/pvr.h
*/
typedef struct {
    int     list_type;          /**< \brief Primitive list
                                     \see   pvr_lists */
    struct {
        int     alpha;          /**< \brief Enable or disable alpha outside modifier
                                     \see   pvr_alpha_switch */
        int     shading;        /**< \brief Shading type
                                     \see   pvr_shading_types */
        int     fog_type;       /**< \brief Fog type outside modifier
                                     \see   pvr_fog_types */
        int     culling;        /**< \brief Culling mode
                                     \see   pvr_cull_modes */
        int     color_clamp;    /**< \brief Color clamp enable/disable outside modifier
                                     \see   pvr_colclamp_switch */
        int     clip_mode;      /**< \brief Clipping mode
                                     \see   pvr_clip_modes */
        int     modifier_mode;  /**< \brief Modifier mode */
        int     specular;       /**< \brief Offset color enable/disable outside modifier
                                     \see   pvr_offset_switch */
        int     alpha2;         /**< \brief Enable/disable alpha inside modifier
                                     \see   pvr_alpha_switch */
        int     fog_type2;      /**< \brief Fog type inside modifier
                                     \see   pvr_fog_types */
        int     color_clamp2;   /**< \brief Color clamp enable/disable inside modifier
                                     \see   pvr_colclamp_switch */
    } gen;                      /**< \brief General parameters */
    struct {
        int     src;            /**< \brief Source blending mode outside modifier
                                     \see   pvr_blend_modes */
        int     dst;            /**< \brief Dest blending mode outside modifier
                                     \see   pvr_blend_modes */
        int     src_enable;     /**< \brief Source blending enable outside modifier
                                     \see   pvr_blend_switch */
        int     dst_enable;     /**< \brief Dest blending enable outside modifier
                                     \see   pvr_blend_switch */
        int     src2;           /**< \brief Source blending mode inside modifier
                                     \see   pvr_blend_modes */
        int     dst2;           /**< \brief Dest blending mode inside modifier
                                     \see   pvr_blend_modes */
        int     src_enable2;    /**< \brief Source blending mode inside modifier
                                     \see   pvr_blend_switch */
        int     dst_enable2;    /**< \brief Dest blending mode inside modifier
                                     \see   pvr_blend_switch */
    } blend;                    /**< \brief Blending parameters */
    struct {
        int     color;          /**< \brief Color format in vertex
                                     \see   pvr_color_fmts */
        int     uv;             /**< \brief U/V data format in vertex
                                     \see   pvr_uv_fmts */
        int     modifier;       /**< \brief Enable or disable modifier effect
                                     \see   pvr_mod_switch */
    } fmt;                      /**< \brief Format control */
    struct {
        int     comparison;     /**< \brief Depth comparison mode
                                     \see pvr_depth_modes */
        int     write;          /**< \brief Enable or disable depth writes
                                     \see pvr_depth_switch */
    } depth;                    /**< \brief Depth comparison/write modes */
    struct {
        int     enable;         /**< \brief Enable/disable texturing
                                     \see   pvr_txr_switch */
        int     filter;         /**< \brief Filtering mode
                                     \see   pvr_filter_modes */
        int     mipmap;         /**< \brief Enable/disable mipmaps
                                     \see   pvr_mip_switch */
        int     mipmap_bias;    /**< \brief Mipmap bias
                                     \see   pvr_mip_bias */
        int     uv_flip;        /**< \brief Enable/disable U/V flipping
                                     \see   pvr_uv_flip */
        int     uv_clamp;       /**< \brief Enable/disable U/V clamping
                                     \see   pvr_uv_clamp */
        int     alpha;          /**< \brief Enable/disable texture alpha
                                     \see   pvr_txralpha_switch */
        int     env;            /**< \brief Texture color contribution
                                     \see   pvr_txrenv_modes */
        int     width;          /**< \brief Texture width (requires a power of 2) */
        int     height;         /**< \brief Texture height (requires a power of 2) */
        int     format;         /**< \brief Texture format
                                     \see   pvr_txr_fmts */
        pvr_ptr_t base;         /**< \brief Texture pointer */
    } txr;                      /**< \brief Texturing params outside modifier */
    struct {
        int     enable;         /**< \brief Enable/disable texturing
                                     \see   pvr_txr_switch */
        int     filter;         /**< \brief Filtering mode
                                     \see   pvr_filter_modes */
        int     mipmap;         /**< \brief Enable/disable mipmaps
                                     \see   pvr_mip_switch */
        int     mipmap_bias;    /**< \brief Mipmap bias
                                     \see   pvr_mip_bias */
        int     uv_flip;        /**< \brief Enable/disable U/V flipping
                                     \see   pvr_uv_flip */
        int     uv_clamp;       /**< \brief Enable/disable U/V clamping
                                     \see   pvr_uv_clamp */
        int     alpha;          /**< \brief Enable/disable texture alpha
                                     \see   pvr_txralpha_switch */
        int     env;            /**< \brief Texture color contribution
                                     \see   pvr_txrenv_modes */
        int     width;          /**< \brief Texture width (requires a power of 2) */
        int     height;         /**< \brief Texture height (requires a power of 2) */
        int     format;         /**< \brief Texture format
                                     \see   pvr_txr_fmts */
        pvr_ptr_t base;         /**< \brief Texture pointer */
    } txr2;                     /**< \brief Texturing params inside modifier */
} pvr_poly_cxt_t;

/** \brief   PVR sprite context.
    \ingroup pvr_ctx

    You should use this more human readable format for specifying your sprite
    contexts, and then compile them into sprite headers when you are ready to
    start using them.

    Unfortunately, it seems that Doxygen chokes up a little bit on this
    structure, and others like it. The documentation should still be mostly
    understandable though...

    \headerfile dc/pvr.h
*/
typedef struct {
    int     list_type;          /**< \brief Primitive list
                                     \see   pvr_lists */
    struct {
        int     alpha;          /**< \brief Enable or disable alpha
                                     \see   pvr_alpha_switch */
        int     fog_type;       /**< \brief Fog type
                                     \see   pvr_fog_types */
        int     culling;        /**< \brief Culling mode
                                     \see   pvr_cull_modes */
        int     color_clamp;    /**< \brief Color clamp enable/disable
                                     \see   pvr_colclamp_switch */
        int     clip_mode;      /**< \brief Clipping mode
                                     \see   pvr_clip_modes */
        int     specular;       /**< \brief Offset color enable/disable
                                     \see   pvr_offset_switch */
    } gen;                      /**< \brief General parameters */
    struct {
        int     src;            /**< \brief Source blending mode
                                     \see   pvr_blend_modes */
        int     dst;            /**< \brief Dest blending mode
                                     \see   pvr_blend_modes */
        int     src_enable;     /**< \brief Source blending enable
                                     \see   pvr_blend_switch */
        int     dst_enable;     /**< \brief Dest blending enable
                                     \see   pvr_blend_switch */
    } blend;
    struct {
        int     comparison;     /**< \brief Depth comparison mode
                                     \see pvr_depth_modes */
        int     write;          /**< \brief Enable or disable depth writes
                                     \see pvr_depth_switch */
    } depth;                    /**< \brief Depth comparison/write modes */
    struct {
        int     enable;         /**< \brief Enable/disable texturing
                                     \see   pvr_txr_switch */
        int     filter;         /**< \brief Filtering mode
                                     \see   pvr_filter_modes */
        int     mipmap;         /**< \brief Enable/disable mipmaps
                                     \see   pvr_mip_switch */
        int     mipmap_bias;    /**< \brief Mipmap bias
                                     \see   pvr_mip_bias */
        int     uv_flip;        /**< \brief Enable/disable U/V flipping
                                     \see   pvr_uv_flip */
        int     uv_clamp;       /**< \brief Enable/disable U/V clamping
                                     \see   pvr_uv_clamp */
        int     alpha;          /**< \brief Enable/disable texture alpha
                                     \see   pvr_txralpha_switch */
        int     env;            /**< \brief Texture color contribution
                                     \see   pvr_txrenv_modes */
        int     width;          /**< \brief Texture width (requires a power of 2) */
        int     height;         /**< \brief Texture height (requires a power of 2) */
        int     format;         /**< \brief Texture format
                                     \see   pvr_txr_fmts */
        pvr_ptr_t base;         /**< \brief Texture pointer */
    } txr;                      /**< \brief Texturing params */
} pvr_sprite_cxt_t;

/* Constants for the above structure; thanks to Benoit Miller for these */

/** \defgroup pvr_lists_types Types
    \brief                    Values of various PVR polygon list types
    \ingroup                  pvr_lists

    Each primitive submitted to the PVR must be placed in one of these lists,
    depending on its characteristics.

    @{
*/
#define PVR_LIST_OP_POLY        0   /**< \brief Opaque polygon list */
#define PVR_LIST_OP_MOD         1   /**< \brief Opaque modifier list */
#define PVR_LIST_TR_POLY        2   /**< \brief Translucent polygon list */
#define PVR_LIST_TR_MOD         3   /**< \brief Translucent modifier list*/
#define PVR_LIST_PT_POLY        4   /**< \brief Punch-thru polygon list */
/** @} */

/** \defgroup pvr_ctx_attrib Attributes
    \brief                   PVR primitive context attributes
    \ingroup                 pvr_ctx
*/

/** \defgroup pvr_shading_types     Shading Modes
    \brief                          PowerVR primitive context shading modes
    \ingroup                        pvr_ctx_attrib

    Each polygon can define how it wants to be shaded, be it with flat or
    Gouraud shading using these constants in the appropriate place in its
    pvr_poly_cxt_t.

    @{
*/
#define PVR_SHADE_FLAT          0   /**< \brief Use flat shading */
#define PVR_SHADE_GOURAUD       1   /**< \brief Use Gouraud shading */
/** @} */

/** \defgroup pvr_ctx_depth     Depth
    \brief                      Depth attributes for PVR polygon contexts
    \ingroup                    pvr_ctx_attrib
*/

/** \defgroup pvr_depth_modes   Comparison Modes
    \brief                      PowerVR depth comparison modes
    \ingroup                    pvr_ctx_depth

    These set the depth function used for comparisons.

    @{
*/
#define PVR_DEPTHCMP_NEVER      0   /**< \brief Never pass */
#define PVR_DEPTHCMP_LESS       1   /**< \brief Less than */
#define PVR_DEPTHCMP_EQUAL      2   /**< \brief Equal to */
#define PVR_DEPTHCMP_LEQUAL     3   /**< \brief Less than or equal to */
#define PVR_DEPTHCMP_GREATER    4   /**< \brief Greater than */
#define PVR_DEPTHCMP_NOTEQUAL   5   /**< \brief Not equal to */
#define PVR_DEPTHCMP_GEQUAL     6   /**< \brief Greater than or equal to */
#define PVR_DEPTHCMP_ALWAYS     7   /**< \brief Always pass */
/** @} */

/** \defgroup pvr_cull_modes        Culling Modes
    \brief                          PowerVR primitive context culling modes
    \ingroup                        pvr_ctx_attrib

    These culling modes can be set by polygons to determine when they are
    culled. They work pretty much as you'd expect them to if you've ever used
    any 3D hardware before.

    @{
*/
#define PVR_CULLING_NONE        0   /**< \brief Disable culling */
#define PVR_CULLING_SMALL       1   /**< \brief Cull if small */
#define PVR_CULLING_CCW         2   /**< \brief Cull if counterclockwise */
#define PVR_CULLING_CW          3   /**< \brief Cull if clockwise */
/** @} */

/** \defgroup pvr_depth_switch      Write Toggle
    \brief                          Enable or Disable Depth Writes.
    \ingroup                        pvr_ctx_depth
    @{
*/
#define PVR_DEPTHWRITE_ENABLE   0   /**< \brief Update the Z value */
#define PVR_DEPTHWRITE_DISABLE  1   /**< \brief Do not update the Z value */
/** @} */

/** \defgroup pvr_ctx_texture Texture
    \brief                    Texture attributes for PVR polygon contexts
    \ingroup                  pvr_ctx_attrib
*/

/** \defgroup pvr_txr_switch        Toggle
    \brief                          Enable or Disable Texturing on Polygons.
    \ingroup                        pvr_ctx_texture
    
    @{
*/
#define PVR_TEXTURE_DISABLE     0   /**< \brief Disable texturing */
#define PVR_TEXTURE_ENABLE      1   /**< \brief Enable texturing */
/** @} */

/** \defgroup pvr_blend             Blending
    \brief                          Blending attributes for PVR primitive contexts
    \ingroup                        pvr_ctx_attrib
*/

/** \defgroup pvr_blend_modes       Blending Modes
    \brief                          Blending modes for PowerVR primitive contexts
    \ingroup                        pvr_blend

    These are all the blending modes that can be done with regard to alpha
    blending on the PVR.

    @{
*/
#define PVR_BLEND_ZERO          0   /**< \brief None of this color */
#define PVR_BLEND_ONE           1   /**< \brief All of this color */
#define PVR_BLEND_DESTCOLOR     2   /**< \brief Destination color */
#define PVR_BLEND_INVDESTCOLOR  3   /**< \brief Inverse of destination color */
#define PVR_BLEND_SRCALPHA      4   /**< \brief Blend with source alpha */
#define PVR_BLEND_INVSRCALPHA   5   /**< \brief Blend with inverse source alpha */
#define PVR_BLEND_DESTALPHA     6   /**< \brief Blend with destination alpha */
#define PVR_BLEND_INVDESTALPHA  7   /**< \brief Blend with inverse destination alpha */
/** @} */

/** \defgroup pvr_blend_switch      Blending Toggle
    \brief                          Enable or Disable Blending.
    \ingroup                        pvr_blend
    
    @{
*/
#define PVR_BLEND_DISABLE       0   /**< \brief Disable blending */
#define PVR_BLEND_ENABLE        1   /**< \brief Enable blending */
/** @} */

/** \defgroup pvr_fog_types         Fog Modes
    \brief                          PowerVR primitive context fog modes
    \ingroup                        pvr_ctx_attrib

    Each polygon can decide what fog type is used with regard to it using these
    constants in its pvr_poly_cxt_t.

    @{
*/
#define PVR_FOG_TABLE           0   /**< \brief Table fog */
#define PVR_FOG_VERTEX          1   /**< \brief Vertex fog */
#define PVR_FOG_DISABLE         2   /**< \brief Disable fog */
#define PVR_FOG_TABLE2          3   /**< \brief Table fog mode 2 */
/** @} */

/** \defgroup pvr_clip_modes        Clipping Modes
    \brief                          PowerVR primitive context clipping modes
    \ingroup                        pvr_ctx_attrib

    These control how primitives are clipped against the user clipping area.

    @{
*/
#define PVR_USERCLIP_DISABLE    0   /**< \brief Disable clipping */
#define PVR_USERCLIP_INSIDE     2   /**< \brief Enable clipping inside area */
#define PVR_USERCLIP_OUTSIDE    3   /**< \brief Enable clipping outside area */
/** @} */

/** \defgroup pvr_ctx_color     Color
    \brief                      Color attributes for PowerVR primitive contexts
    \ingroup                    pvr_ctx_attrib
*/

/** \defgroup pvr_colclamp_switch   Clamping Toggle
    \brief                          Enable or Disable Color Clamping
    \ingroup                        pvr_ctx_color

    Enabling color clamping will clamp colors between the minimum and maximum
    values before any sort of fog processing.

    @{
*/
#define PVR_CLRCLAMP_DISABLE    0   /**< \brief Disable color clamping */
#define PVR_CLRCLAMP_ENABLE     1   /**< \brief Enable color clamping */
/** @} */

/** \defgroup pvr_offset_switch     Offset Toggle
    \brief                          Enable or Disable Offset Color
    \ingroup                        pvr_ctx_color

    Enabling offset color calculation allows for "specular" like effects on a
    per-vertex basis, by providing an additive color in the calculation of the
    final pixel colors. In vertex types with a "oargb" parameter, that's what it
    is for.

    \note
    This must be enabled for bumpmap polygons in order to allow you to
    specify the parameters in the oargb field of the vertices.

    @{
*/
#define PVR_SPECULAR_DISABLE    0   /**< \brief Disable offset colors */
#define PVR_SPECULAR_ENABLE     1   /**< \brief Enable offset colors */
/** @} */

/** \defgroup pvr_alpha_switch      Alpha Toggle
    \brief                          Enable or Disable Alpha Blending
    \ingroup                        pvr_blend

    This causes the alpha value in the vertex color to be paid attention to. It
    really only makes sense to enable this for translucent or punch-thru polys.

    @{
*/
#define PVR_ALPHA_DISABLE       0   /**< \brief Disable alpha blending */
#define PVR_ALPHA_ENABLE        1   /**< \brief Enable alpha blending */
/** @} */

/** \defgroup pvr_txralpha_switch   Alpha Toggle
    \brief                          Enable or Disable Texture Alpha Blending
    \ingroup                        pvr_ctx_texture

    This causes the alpha value in the texel color to be paid attention to. It
    really only makes sense to enable this for translucent or punch-thru polys.

    @{
*/
#define PVR_TXRALPHA_ENABLE     0   /**< \brief Enable alpha blending */
#define PVR_TXRALPHA_DISABLE    1   /**< \brief Disable alpha blending */
/** @} */

/** \defgroup pvr_uv_flip           U/V Flip Mode
    \brief                          Enable or disable U/V flipping on the PVR
    \ingroup                        pvr_ctx_texture

    These flags determine what happens when U/V coordinate values exceed 1.0.
    In any of the flipped cases, the specified coordinate value will flip around
    after 1.0, essentially mirroring the image. So, if you displayed an image
    with a U coordinate of 0.0 on the left hand side and 2.0 on the right hand
    side with U flipping turned on, you'd have an image that was displayed twice
    as if mirrored across the middle. This mirroring behavior happens at every
    unit boundary (so at 2.0 it returns to normal, at 3.0 it flips, etc).

    The default case is to disable mirroring. In addition, clamping of the U/V
    coordinates by PVR_UVCLAMP_U, PVR_UVCLAMP_V, or PVR_UVCLAMP_UV will disable
    the mirroring behavior.
    @{
*/
#define PVR_UVFLIP_NONE         0   /**< \brief No flipped coordinates */
#define PVR_UVFLIP_V            1   /**< \brief Flip V only */
#define PVR_UVFLIP_U            2   /**< \brief Flip U only */
#define PVR_UVFLIP_UV           3   /**< \brief Flip U and V */
/** @} */

/** \defgroup pvr_uv_clamp  U/V Clamp Mode
    \brief                  Enable or disable clamping of U/V on the PVR
    \ingroup                pvr_ctx_texture

    These flags determine whether clamping will be applied to U/V coordinate
    values that exceed 1.0. If enabled, these modes will explicitly override the
    flip/mirroring modes (PVR_UVFLIP_U, PVR_UVFLIP_V, and PVR_UVFLIP_UV), and
    will instead ensure that the coordinate(s) in question never exceed 1.0.
    @{
*/
#define PVR_UVCLAMP_NONE        0   /**< \brief Disable clamping */
#define PVR_UVCLAMP_V           1   /**< \brief Clamp V only */
#define PVR_UVCLAMP_U           2   /**< \brief Clamp U only */
#define PVR_UVCLAMP_UV          3   /**< \brief Clamp U and V */
/** @} */

/** \defgroup pvr_filter_modes      Sampling Modes
    \brief                          PowerVR texture sampling modes
    \ingroup                        pvr_ctx_texture

    @{
*/
#define PVR_FILTER_NONE         0   /**< \brief No filtering (point sample) */
#define PVR_FILTER_NEAREST      0   /**< \brief No filtering (point sample) */
#define PVR_FILTER_BILINEAR     2   /**< \brief Bilinear interpolation */
#define PVR_FILTER_TRILINEAR1   4   /**< \brief Trilinear interpolation pass 1 */
#define PVR_FILTER_TRILINEAR2   6   /**< \brief Trilinear interpolation pass 2 */
/** @} */

/** \defgroup pvr_mip_bias          Mipmap Bias Modes
    \brief                          Mipmap bias modes for PowerVR primitive contexts
    \ingroup                        pvr_ctx_texture

    @{
*/
#define PVR_MIPBIAS_NORMAL      PVR_MIPBIAS_1_00    /* txr_mipmap_bias */
#define PVR_MIPBIAS_0_25        1
#define PVR_MIPBIAS_0_50        2
#define PVR_MIPBIAS_0_75        3
#define PVR_MIPBIAS_1_00        4
#define PVR_MIPBIAS_1_25        5
#define PVR_MIPBIAS_1_50        6
#define PVR_MIPBIAS_1_75        7
#define PVR_MIPBIAS_2_00        8
#define PVR_MIPBIAS_2_25        9
#define PVR_MIPBIAS_2_50        10
#define PVR_MIPBIAS_2_75        11
#define PVR_MIPBIAS_3_00        12
#define PVR_MIPBIAS_3_25        13
#define PVR_MIPBIAS_3_50        14
#define PVR_MIPBIAS_3_75        15
/** @} */

/** \defgroup pvr_txrenv_modes      Color Calculation Modes
    \brief                          PowerVR texture color calculation modes
    \ingroup                        pvr_ctx_texture

    @{
*/
#define PVR_TXRENV_REPLACE          0   /**< \brief C = Ct, A = At */
#define PVR_TXRENV_MODULATE         1   /**< \brief  C = Cs * Ct, A = At */
#define PVR_TXRENV_DECAL            2   /**< \brief C = (Cs * At) + (Cs * (1-At)), A = As */
#define PVR_TXRENV_MODULATEALPHA    3   /**< \brief C = Cs * Ct, A = As * At */
/** @} */

/** \defgroup pvr_mip_switch        Mipmap Toggle
    \brief                          Enable or Disable Mipmap Processing
    \ingroup                        pvr_ctx_texture

    @{
*/
#define PVR_MIPMAP_DISABLE      0   /**< \brief Disable mipmap processing */
#define PVR_MIPMAP_ENABLE       1   /**< \brief Enable mipmap processing */
/** @} */

/** \defgroup pvr_txr_fmts          Formats
    \brief                          PowerVR texture formats
    \ingroup                        pvr_txr_mgmt

    These are the texture formats that the PVR supports. Note that some of
    these, you can OR together with other values.

    @{
*/
#define PVR_TXRFMT_NONE         0           /**< \brief No texture */
#define PVR_TXRFMT_VQ_DISABLE   (0 << 30)   /**< \brief Not VQ encoded */
#define PVR_TXRFMT_VQ_ENABLE    (1 << 30)   /**< \brief VQ encoded */
#define PVR_TXRFMT_ARGB1555     (0 << 27)   /**< \brief 16-bit ARGB1555 */
#define PVR_TXRFMT_RGB565       (1 << 27)   /**< \brief 16-bit RGB565 */
#define PVR_TXRFMT_ARGB4444     (2 << 27)   /**< \brief 16-bit ARGB4444 */
#define PVR_TXRFMT_YUV422       (3 << 27)   /**< \brief YUV422 format */
#define PVR_TXRFMT_BUMP         (4 << 27)   /**< \brief Bumpmap format */
#define PVR_TXRFMT_PAL4BPP      (5 << 27)   /**< \brief 4BPP paletted format */
#define PVR_TXRFMT_PAL8BPP      (6 << 27)   /**< \brief 8BPP paletted format */
#define PVR_TXRFMT_TWIDDLED     (0 << 26)   /**< \brief Texture is twiddled */
#define PVR_TXRFMT_NONTWIDDLED  (1 << 26)   /**< \brief Texture is not twiddled */
#define PVR_TXRFMT_NOSTRIDE     (0 << 21)   /**< \brief Texture is not strided */
#define PVR_TXRFMT_STRIDE       (1 << 21)   /**< \brief Texture is strided */

/* OR one of these into your texture format if you need it. Note that
   these coincide with the twiddled/stride bits, so you can't have a
   non-twiddled/strided texture that's paletted! */

/** \brief   8BPP palette selector

    \param  x               The palette index */
#define PVR_TXRFMT_8BPP_PAL(x)  ((x) << 25)

/** \brief   4BPP palette selector

    \param  x               The palette index */
#define PVR_TXRFMT_4BPP_PAL(x)  ((x) << 21)
/** @} */

/** \defgroup pvr_color_fmts        Vertex Formats
    \brief                          Color formats for PowerVR vertices
    \ingroup                        pvr_ctx_color

    These control how colors are represented in polygon data.

    @{
*/
#define PVR_CLRFMT_ARGBPACKED       0   /**< \brief 32-bit integer ARGB */
#define PVR_CLRFMT_4FLOATS          1   /**< \brief 4 floating point values */
#define PVR_CLRFMT_INTENSITY        2   /**< \brief Intensity color */
#define PVR_CLRFMT_INTENSITY_PREV   3   /**< \brief Use last intensity */
/** @} */

/** \defgroup pvr_uv_fmts           U/V Data Format
    \brief                          U/V data format for PVR textures
    \ingroup                        pvr_ctx_texture
    @{
*/
#define PVR_UVFMT_32BIT         0   /**< \brief 32-bit floating point U/V */
#define PVR_UVFMT_16BIT         1   /**< \brief 16-bit floating point U/V */
/** @} */

/** \defgroup pvr_ctx_modvol        Modifier Volumes
    \brief                          PowerVR modifier volume polygon context attributes
    \ingroup                        pvr_ctx_attrib
*/

/** \defgroup pvr_mod_switch        Toggle
    \brief                          Enable or Disable Modifier Effects
    \ingroup                        pvr_ctx_modvol
    @{
*/
#define PVR_MODIFIER_DISABLE    0   /**< \brief Disable modifier effects */
#define PVR_MODIFIER_ENABLE     1   /**< \brief Enable modifier effects */
/** @} */

/** \defgroup pvr_mod_types         Types
    \brief                          Modifier volume types for PowerVR primitive contexts
    \ingroup                        pvr_ctx_modvol
    @{
*/
#define PVR_MODIFIER_CHEAP_SHADOW   0
#define PVR_MODIFIER_NORMAL         1
/** @} */

/** \defgroup pvr_mod_modes         Modes
    \brief                          Modifier volume modes for PowerVR primitive contexts
    \ingroup                        pvr_ctx_modvol

    All triangles in a single modifier volume should be of the other poly type,
    except for the last one. That should be either of the other two types,
    depending on whether you want an inclusion or exclusion volume.

    @{
*/
#define PVR_MODIFIER_OTHER_POLY         0   /**< \brief Not the last polygon in the volume */
#define PVR_MODIFIER_INCLUDE_LAST_POLY  1   /**< \brief Last polygon, inclusion volume */
#define PVR_MODIFIER_EXCLUDE_LAST_POLY  2   /**< \brief Last polygon, exclusion volume */
/** @} */

/** \defgroup pvr_primitives_headers Headers
    \brief                           Compiled headers for polygons and sprites
    \ingroup pvr_primitives

    @{
*/

/** \brief   PVR polygon header.

    This is the hardware equivalent of a rendering context; you'll create one of
    these from your pvr_poly_cxt_t and use it for submission to the hardware.

    \headerfile dc/pvr.h
*/
typedef struct pvr_poly_hdr {
    alignas(32)
    uint32_t cmd;                /**< \brief TA command */
    uint32_t mode1;              /**< \brief Parameter word 1 */
    uint32_t mode2;              /**< \brief Parameter word 2 */
    uint32_t mode3;              /**< \brief Parameter word 3 */
    uint32_t d1;                 /**< \brief Dummy value */
    uint32_t d2;                 /**< \brief Dummy value */
    uint32_t d3;                 /**< \brief Dummy value */
    uint32_t d4;                 /**< \brief Dummy value */
} pvr_poly_hdr_t;

/** \brief   PVR polygon header with intensity color.

    This is the equivalent of pvr_poly_hdr_t, but for use with intensity color.

    \headerfile dc/pvr.h
*/
typedef struct pvr_poly_ic_hdr {
    alignas(32)
    uint32_t cmd;                /**< \brief TA command */
    uint32_t mode1;              /**< \brief Parameter word 1 */
    uint32_t mode2;              /**< \brief Parameter word 2 */
    uint32_t mode3;              /**< \brief Parameter word 3 */
    float   a;                   /**< \brief Face color alpha component */
    float   r;                   /**< \brief Face color red component */
    float   g;                   /**< \brief Face color green component */
    float   b;                   /**< \brief Face color blue component */
} pvr_poly_ic_hdr_t;

/** \brief   PVR polygon header to be used with modifier volumes.

    This is the equivalent of a pvr_poly_hdr_t for use when a polygon is to be
    used with modifier volumes.

    \headerfile dc/pvr.h
*/
typedef struct pvr_poly_mod_hdr {
    alignas(32)
    uint32_t cmd;                /**< \brief TA command */
    uint32_t mode1;              /**< \brief Parameter word 1 */
    uint32_t mode2_0;            /**< \brief Parameter word 2 (outside volume) */
    uint32_t mode3_0;            /**< \brief Parameter word 3 (outside volume) */
    uint32_t mode2_1;            /**< \brief Parameter word 2 (inside volume) */
    uint32_t mode3_1;            /**< \brief Parameter word 3 (inside volume) */
    uint32_t d1;                 /**< \brief Dummy value */
    uint32_t d2;                 /**< \brief Dummy value */
} pvr_poly_mod_hdr_t;

/** \brief   PVR polygon header specifically for sprites.

    This is the equivalent of a pvr_poly_hdr_t for use when a quad/sprite is to
    be rendered. Note that the color data is here, not in the vertices.

    \headerfile dc/pvr.h
*/
typedef struct pvr_sprite_hdr {
    alignas(32)
    uint32_t cmd;                /**< \brief TA command */
    uint32_t mode1;              /**< \brief Parameter word 1 */
    uint32_t mode2;              /**< \brief Parameter word 2 */
    uint32_t mode3;              /**< \brief Parameter word 3 */
    uint32_t argb;               /**< \brief Sprite face color */
    uint32_t oargb;              /**< \brief Sprite offset color */
    uint32_t d1;                 /**< \brief Dummy value */
    uint32_t d2;                 /**< \brief Dummy value */
} pvr_sprite_hdr_t;

/** \brief   Modifier volume header.

    This is the header that should be submitted when dealing with setting a
    modifier volume.

    \headerfile dc/pvr.h
*/
typedef struct pvr_mod_hdr {
    alignas(32)
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

/** \brief   Generic PVR vertex type.

    The PVR chip itself supports many more vertex types, but this is the main
    one that can be used with both textured and non-textured polygons, and is
    fairly fast.

    \headerfile dc/pvr.h
*/
typedef struct pvr_vertex {
    alignas(32)
    uint32_t flags;              /**< \brief TA command (vertex flags) */
    float   x;                   /**< \brief X coordinate */
    float   y;                   /**< \brief Y coordinate */
    float   z;                   /**< \brief Z coordinate */
    float   u;                   /**< \brief Texture U coordinate */
    float   v;                   /**< \brief Texture V coordinate */
    uint32_t argb;               /**< \brief Vertex color */
    uint32_t oargb;              /**< \brief Vertex offset color */
} pvr_vertex_t;

/** \brief   PVR vertex type: Non-textured, packed color, affected by modifier
             volume.

    This vertex type has two copies of colors. The second color is used when
    enclosed within a modifier volume.

    \headerfile dc/pvr.h
*/
typedef struct pvr_vertex_pcm {
    alignas(32)
    uint32_t flags;              /**< \brief TA command (vertex flags) */
    float   x;                   /**< \brief X coordinate */
    float   y;                   /**< \brief Y coordinate */
    float   z;                   /**< \brief Z coordinate */
    uint32_t argb0;              /**< \brief Vertex color (outside volume) */
    uint32_t argb1;              /**< \brief Vertex color (inside volume) */
    uint32_t d1;                 /**< \brief Dummy value */
    uint32_t d2;                 /**< \brief Dummy value */
} pvr_vertex_pcm_t;

/** \brief   PVR vertex type: Textured, packed color, affected by modifier volume.

    Note that this vertex type has two copies of colors, offset colors, and
    texture coords. The second set of texture coords, colors, and offset colors
    are used when enclosed within a modifier volume.

    \headerfile dc/pvr.h
*/
typedef struct pvr_vertex_tpcm {
    alignas(32)
    uint32_t flags;              /**< \brief TA command (vertex flags) */
    float   x;                   /**< \brief X coordinate */
    float   y;                   /**< \brief Y coordinate */
    float   z;                   /**< \brief Z coordinate */
    float   u0;                  /**< \brief Texture U coordinate (outside) */
    float   v0;                  /**< \brief Texture V coordinate (outside) */
    uint32_t argb0;              /**< \brief Vertex color (outside) */
    uint32_t oargb0;             /**< \brief Vertex offset color (outside) */
    float   u1;                  /**< \brief Texture U coordinate (inside) */
    float   v1;                  /**< \brief Texture V coordinate (inside) */
    uint32_t argb1;              /**< \brief Vertex color (inside) */
    uint32_t oargb1;             /**< \brief Vertex offset color (inside) */
    uint32_t d1;                 /**< \brief Dummy value */
    uint32_t d2;                 /**< \brief Dummy value */
    uint32_t d3;                 /**< \brief Dummy value */
    uint32_t d4;                 /**< \brief Dummy value */
} pvr_vertex_tpcm_t;

/** \brief   PVR vertex type: Textured sprite.

    This vertex type is to be used with the sprite polygon header and the sprite
    related commands to draw textured sprites. Note that there is no fourth Z
    coordinate. I suppose it just gets interpolated?

    The U/V coordinates in here are in the 16-bit per coordinate form. Also,
    like the fourth Z value, there is no fourth U or V, so it must get
    interpolated from the others.

    \headerfile dc/pvr.h
*/
typedef struct pvr_sprite_txr {
    alignas(32)
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

/** \brief   PVR vertex type: Untextured sprite.

    This vertex type is to be used with the sprite polygon header and the sprite
    related commands to draw untextured sprites (aka, quads).
*/
typedef struct pvr_sprite_col {
    alignas(32)
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

/** \brief   PVR vertex type: Modifier volume.

    This vertex type is to be used with the modifier volume header to specify
    triangular modifier areas.
*/
typedef struct pvr_modifier_vol {
    alignas(32)
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

/** \brief   Pack four floating point color values into a 32-bit integer form.

    All of the color values should be between 0 and 1.

    \param  a               Alpha value
    \param  r               Red value
    \param  g               Green value
    \param  b               Blue value
    \return                 The packed color value
*/
#define PVR_PACK_COLOR(a, r, g, b) ( \
                                     ( ((uint8_t)( (a) * 255 ) ) << 24 ) | \
                                     ( ((uint8_t)( (r) * 255 ) ) << 16 ) | \
                                     ( ((uint8_t)( (g) * 255 ) ) << 8 ) | \
                                     ( ((uint8_t)( (b) * 255 ) ) << 0 ) )

/** \brief   Pack two floating point coordinates into one 32-bit value,
             truncating them to 16-bits each.

    \param  u               First coordinate to pack
    \param  v               Second coordinate to pack
    \return                 The packed coordinates
*/
static inline uint32_t PVR_PACK_16BIT_UV(float u, float v) {
    union {
        float f;
        uint32_t i;
    } u2, v2;

    u2.f = u;
    v2.f = v;

    return (u2.i & 0xFFFF0000) | (v2.i >> 16);
}

/** @} */

/** \defgroup pvr_commands          TA Command Values
    \brief                          Command values for submitting data to the TA
    \ingroup                        pvr_primitives_headers

    These are are appropriate values for TA commands. Use whatever goes with the
    primitive type you're using.

    @{
*/
#define PVR_CMD_POLYHDR     0x80840000  /**< \brief PVR polygon header.
Striplength set to 2 */
#define PVR_CMD_VERTEX      0xe0000000  /**< \brief PVR vertex data */
#define PVR_CMD_VERTEX_EOL  0xf0000000  /**< \brief PVR vertex, end of strip */
#define PVR_CMD_USERCLIP    0x20000000  /**< \brief PVR user clipping area */
#define PVR_CMD_MODIFIER    0x80000000  /**< \brief PVR modifier volume */
#define PVR_CMD_SPRITE      0xA0000000  /**< \brief PVR sprite header */
/** @} */

/** \cond
    Deprecated macros, replaced by the pvr_bitmasks macros below.
 */
#define PVR_TA_CMD_TYPE_SHIFT       24
#define PVR_TA_CMD_TYPE_MASK        (7 << PVR_TA_CMD_TYPE_SHIFT)

#define PVR_TA_CMD_USERCLIP_SHIFT   16
#define PVR_TA_CMD_USERCLIP_MASK    (3 << PVR_TA_CMD_USERCLIP_SHIFT)

#define PVR_TA_CMD_CLRFMT_SHIFT     4
#define PVR_TA_CMD_CLRFMT_MASK      (7 << PVR_TA_CMD_CLRFMT_SHIFT)

#define PVR_TA_CMD_SPECULAR_SHIFT   2
#define PVR_TA_CMD_SPECULAR_MASK    (1 << PVR_TA_CMD_SPECULAR_SHIFT)

#define PVR_TA_CMD_SHADE_SHIFT      1
#define PVR_TA_CMD_SHADE_MASK       (1 << PVR_TA_CMD_SHADE_SHIFT)

#define PVR_TA_CMD_UVFMT_SHIFT      0
#define PVR_TA_CMD_UVFMT_MASK       (1 << PVR_TA_CMD_UVFMT_SHIFT)

#define PVR_TA_CMD_MODIFIER_SHIFT   7
#define PVR_TA_CMD_MODIFIER_MASK    (1 <<  PVR_TA_CMD_MODIFIER_SHIFT)

#define PVR_TA_CMD_MODIFIERMODE_SHIFT   6
#define PVR_TA_CMD_MODIFIERMODE_MASK    (1 <<  PVR_TA_CMD_MODIFIERMODE_SHIFT)

#define PVR_TA_PM1_DEPTHCMP_SHIFT   29
#define PVR_TA_PM1_DEPTHCMP_MASK    (7 << PVR_TA_PM1_DEPTHCMP_SHIFT)

#define PVR_TA_PM1_CULLING_SHIFT    27
#define PVR_TA_PM1_CULLING_MASK     (3 << PVR_TA_PM1_CULLING_SHIFT)

#define PVR_TA_PM1_DEPTHWRITE_SHIFT 26
#define PVR_TA_PM1_DEPTHWRITE_MASK  (1 << PVR_TA_PM1_DEPTHWRITE_SHIFT)

#define PVR_TA_PM1_TXRENABLE_SHIFT  25
#define PVR_TA_PM1_TXRENABLE_MASK   (1 << PVR_TA_PM1_TXRENABLE_SHIFT)

#define PVR_TA_PM1_MODIFIERINST_SHIFT   29
#define PVR_TA_PM1_MODIFIERINST_MASK    (3 <<  PVR_TA_PM1_MODIFIERINST_SHIFT)

#define PVR_TA_PM2_SRCBLEND_SHIFT   29
#define PVR_TA_PM2_SRCBLEND_MASK    (7 << PVR_TA_PM2_SRCBLEND_SHIFT)

#define PVR_TA_PM2_DSTBLEND_SHIFT   26
#define PVR_TA_PM2_DSTBLEND_MASK    (7 << PVR_TA_PM2_DSTBLEND_SHIFT)

#define PVR_TA_PM2_SRCENABLE_SHIFT  25
#define PVR_TA_PM2_SRCENABLE_MASK   (1 << PVR_TA_PM2_SRCENABLE_SHIFT)

#define PVR_TA_PM2_DSTENABLE_SHIFT  24
#define PVR_TA_PM2_DSTENABLE_MASK   (1 << PVR_TA_PM2_DSTENABLE_SHIFT)

#define PVR_TA_PM2_FOG_SHIFT        22
#define PVR_TA_PM2_FOG_MASK     (3 << PVR_TA_PM2_FOG_SHIFT)

#define PVR_TA_PM2_CLAMP_SHIFT      21
#define PVR_TA_PM2_CLAMP_MASK       (1 << PVR_TA_PM2_CLAMP_SHIFT)

#define PVR_TA_PM2_ALPHA_SHIFT      20
#define PVR_TA_PM2_ALPHA_MASK       (1 << PVR_TA_PM2_ALPHA_SHIFT)

#define PVR_TA_PM2_TXRALPHA_SHIFT   19
#define PVR_TA_PM2_TXRALPHA_MASK    (1 << PVR_TA_PM2_TXRALPHA_SHIFT)

#define PVR_TA_PM2_UVFLIP_SHIFT     17
#define PVR_TA_PM2_UVFLIP_MASK      (3 << PVR_TA_PM2_UVFLIP_SHIFT)

#define PVR_TA_PM2_UVCLAMP_SHIFT    15
#define PVR_TA_PM2_UVCLAMP_MASK     (3 << PVR_TA_PM2_UVCLAMP_SHIFT)

#define PVR_TA_PM2_FILTER_SHIFT     12
#define PVR_TA_PM2_FILTER_MASK      (7 << PVR_TA_PM2_FILTER_SHIFT)

#define PVR_TA_PM2_MIPBIAS_SHIFT    8
#define PVR_TA_PM2_MIPBIAS_MASK     (15 << PVR_TA_PM2_MIPBIAS_SHIFT)

#define PVR_TA_PM2_TXRENV_SHIFT     6
#define PVR_TA_PM2_TXRENV_MASK      (3 << PVR_TA_PM2_TXRENV_SHIFT)

#define PVR_TA_PM2_USIZE_SHIFT      3
#define PVR_TA_PM2_USIZE_MASK       (7 << PVR_TA_PM2_USIZE_SHIFT)

#define PVR_TA_PM2_VSIZE_SHIFT      0
#define PVR_TA_PM2_VSIZE_MASK       (7 << PVR_TA_PM2_VSIZE_SHIFT)

#define PVR_TA_PM3_MIPMAP_SHIFT     31
#define PVR_TA_PM3_MIPMAP_MASK      (1 << PVR_TA_PM3_MIPMAP_SHIFT)

#define PVR_TA_PM3_TXRFMT_SHIFT     0
#define PVR_TA_PM3_TXRFMT_MASK      0xffffffff
/** \endcond */

/** \defgroup pvr_bitmasks          Constants and Masks
    \brief                          Polygon header constants and masks
    \ingroup                        pvr_primitives_headers

    Note that thanks to the arrangement of constants, this is mainly a matter of
    bit shifting to compile headers...

    @{
*/
#define PVR_TA_CMD_TYPE            GENMASK(26, 24)
#define PVR_TA_CMD_USERCLIP        GENMASK(17, 16)
#define PVR_TA_CMD_MODIFIER        BIT(7)
#define PVR_TA_CMD_MODIFIERMODE    BIT(6)
#define PVR_TA_CMD_CLRFMT          GENMASK(5, 4)
#define PVR_TA_CMD_TXRENABLE       BIT(3)
#define PVR_TA_CMD_SPECULAR        BIT(2)
#define PVR_TA_CMD_SHADE           BIT(1)
#define PVR_TA_CMD_UVFMT           BIT(0)
#define PVR_TA_PM1_DEPTHCMP        GENMASK(31, 29)
#define PVR_TA_PM1_CULLING         GENMASK(28, 27)
#define PVR_TA_PM1_DEPTHWRITE      BIT(26)
#define PVR_TA_PM1_TXRENABLE       BIT(25)
#define PVR_TA_PM1_MODIFIERINST    GENMASK(30, 29)
#define PVR_TA_PM2_SRCBLEND        GENMASK(31, 29)
#define PVR_TA_PM2_DSTBLEND        GENMASK(28, 26)
#define PVR_TA_PM2_SRCENABLE       BIT(25)
#define PVR_TA_PM2_DSTENABLE       BIT(24)
#define PVR_TA_PM2_FOG             GENMASK(23, 22)
#define PVR_TA_PM2_CLAMP           BIT(21)
#define PVR_TA_PM2_ALPHA           BIT(20)
#define PVR_TA_PM2_TXRALPHA        BIT(19)
#define PVR_TA_PM2_UVFLIP          GENMASK(18, 17)
#define PVR_TA_PM2_UVCLAMP         GENMASK(16, 15)
#define PVR_TA_PM2_FILTER          GENMASK(14, 12)
#define PVR_TA_PM2_MIPBIAS         GENMASK(11, 8)
#define PVR_TA_PM2_TXRENV          GENMASK(7, 6)
#define PVR_TA_PM2_USIZE           GENMASK(5, 3)
#define PVR_TA_PM2_VSIZE           GENMASK(2, 0)
#define PVR_TA_PM3_MIPMAP          BIT(31)
#define PVR_TA_PM3_TXRFMT          GENMASK(30, 21)
/** @} */

/* Initialization ****************************************************/
/** \defgroup pvr_init  Initialization 
    \brief              Driver initialization and shutdown
    \ingroup            pvr

    Initialization and shutdown: stuff you should only ever have to do
    once in your program. 
*/

/** \defgroup pvr_binsizes          Primitive Bin Sizes
    \brief                          Available sizes for primitive bins
    \ingroup                        pvr_init
    @{
*/
#define PVR_BINSIZE_0   0   /**< \brief 0-length (disables the list) */
#define PVR_BINSIZE_8   8   /**< \brief 8-word (32-byte) length */
#define PVR_BINSIZE_16  16  /**< \brief 16-word (64-byte) length */
#define PVR_BINSIZE_32  32  /**< \brief 32-word (128-byte) length */
/** @} */

/** \brief   PVR initialization structure
    \ingroup pvr_init

    This structure defines how the PVR initializes various parts of the system,
    including the primitive bin sizes, the vertex buffer size, and whether
    vertex DMA will be enabled.

    You essentially fill one of these in, and pass it to pvr_init().

    \headerfile dc/pvr.h
*/
typedef struct {
    /** \brief  Bin sizes.

        The bins go in the following order: opaque polygons, opaque modifiers,
        translucent polygons, translucent modifiers, punch-thrus
    */
    int     opb_sizes[5];

    /** \brief  Vertex buffer size (should be a nice round number) */
    int     vertex_buf_size;

    /** \brief  Enable vertex DMA?

        Set to non-zero if we want to enable vertex DMA mode. Note that if this
        is set, then _all_ enabled lists need to have a vertex buffer assigned,
        even if you never use that list for anything.
    */
    int     dma_enabled;

    /** \brief  Enable horizontal scaling?

        Set to non-zero if horizontal scaling is to be enabled. By enabling this
        setting and stretching your image to double the native screen width, you
        can get horizontal full-screen anti-aliasing. */
    int     fsaa_enabled;

    /** \brief  Disable translucent polygon autosort?

        Set to non-zero to disable translucent polygon autosorting. By enabling
        this setting, the PVR acts more like a traditional Z-buffered system
        when rendering translucent polygons, meaning you must pre-sort them
        yourself if you want them to appear in the right order. */
    int     autosort_disabled;


    /** \brief  OPB Overflow Count.

        Preallocates this many extra OPBs (sets of tile bins), allowing the PVR
        to use the extra space when there's too much geometry in the first OPB.
    
        Increasing this value can eliminate artifacts where pieces of geometry
        flicker in and out of existence along the tile boundaries. */

    int     opb_overflow_count;

    /** \brief  Disable vertex buffer double-buffering.

        Use only one single vertex buffer. This means that the PVR must finish
        rendering before the Tile Accelerator is used to prepare a new frame;
        but it allows using much smaller vertex buffers. */
    int     vbuf_doublebuf_disabled;

} pvr_init_params_t;

/** \brief   Initialize the PVR chip to ready status.
    \ingroup pvr_init

    This function enables the specified lists and uses the specified parameters.
    Note that bins and vertex buffers come from the texture memory pool, so only
    allocate what you actually need. Expects that a 2D mode was initialized
    already using the vid_* API.

    \param  params          The set of parameters to initialize with
    \retval 0               On success
    \retval -1              If the PVR has already been initialized or the video
                            mode active is not suitable for 3D
*/
int pvr_init(const pvr_init_params_t *params);

/** \brief   Simple PVR initialization.
    \ingroup pvr_init

    This simpler function initializes the PVR using 16/16 for the opaque
    and translucent lists' bin sizes, and 0's for everything else. It sets 512KB
    of vertex buffer. This is equivalent to the old ta_init_defaults() for now.

    \retval 0               On success
    \retval -1              If the PVR has already been initialized or the video
                            mode active is not suitable for 3D
*/
int pvr_init_defaults(void);

/** \brief   Shut down the PVR chip from ready status.
    \ingroup pvr_init

    This essentially leaves the video system in 2D mode as it was before the
    init.

    \retval 0               On success
    \retval -1              If the PVR has not been initialized
*/
int pvr_shutdown(void);


/* Misc parameters ***************************************************/

/** \defgroup pvr_global Global State
    \brief               PowerVR functionality which is managed globally
    \ingroup             pvr

    These are miscellaneous parameters you can set which affect the
    rendering process.
*/

/** \brief   Set the background plane color.
    \ingroup pvr_global

    This function sets the color of the area of the screen not covered by any
    other polygons.

    \param  r               Red component of the color to set
    \param  g               Green component of the color to set
    \param  b               Blue component of the color to set
*/
void pvr_set_bg_color(float r, float g, float b);

/** \brief   Set cheap shadow parameters.
    \ingroup pvr_global

    This function sets up the PVR cheap shadow parameters for use. You can only
    specify one scale value per frame, so the effect that you can get from this
    is somewhat limited, but if you want simple shadows, this is the easiest way
    to do it.

    Polygons affected by a shadow modifier volume will effectively multiply
    their final color by the scale value set here when shadows are enabled and
    the polygon is inside the modifier (or outside for exclusion volumes).

    \param  enable          Set to true to enable cheap shadow mode.
    \param  scale_value     Floating point value (between 0 and 1) representing
                            how colors of polygons affected by and inside the
                            volume will be modified by the shadow volume.
*/
void pvr_set_shadow_scale(bool enable, float scale_value);

/** \brief   Set Z clipping depth.
    \ingroup pvr_global

    This function sets the Z clipping depth. The default value for this is
    0.0001.

    \param  zc              The new value to set the z clip parameter to.
*/
void pvr_set_zclip(float zc);

/** \brief   Retrieve the current VBlank count.
    \ingroup pvr_stats

    This function retrieves the number of VBlank interrupts that have occurred
    since the PVR was initialized.

    \return                 The number of VBlanks since init
*/
int pvr_get_vbl_count(void);

/** \defgroup pvr_stats         Profiling
    \brief                      Rendering stats and metrics for profiling
    \ingroup                    pvr
*/

/** \brief   PVR statistics structure.
    \ingroup pvr_stats

    This structure is used to hold various statistics about the operation of the
    PVR since initialization.

    \headerfile dc/pvr.h
*/
typedef struct pvr_stats {
    uint64_t frame_last_time;     /**< \brief Ready-to-Ready length for the last frame in nanoseconds */
    uint64_t reg_last_time;       /**< \brief Registration time for the last frame in nanoseconds */
    uint64_t rnd_last_time;       /**< \brief Rendering time for the last frame in nanoseconds */
    uint64_t buf_last_time;       /**< \brief DMA buffer file time for the last frame in nanoseconds */
    size_t   frame_count;         /**< \brief Total number of rendered/viewed frames */
    size_t   vbl_count;           /**< \brief VBlank count */
    size_t   vtx_buffer_used;     /**< \brief Number of bytes used in the vertex buffer for the last frame */
    size_t   vtx_buffer_used_max; /**< \brief Number of bytes used in the vertex buffer for the largest frame */
    float    frame_rate;          /**< \brief Current frame rate (per second) */
    uint32_t enabled_list_mask;   /**< \brief Which lists are enabled? */
    /* ... more later as it's implemented ... */
} pvr_stats_t;

/** \brief   Get the current statistics from the PVR.
    \ingroup pvr_stats

    This function fills in the pvr_stats_t structure passed in with the current
    statistics of the system.

    \param  stat            The statistics structure to fill in. Must not be
                            NULL
    \retval 0               On success
    \retval -1              If the PVR is not initialized
*/
int pvr_get_stats(pvr_stats_t *stat);

/* Scene rendering ***************************************************/
/** \defgroup   pvr_scene_mgmt  Scene Submission
    \brief                      PowerVR API for submitting scene geometry
    \ingroup                    pvr

    This API is used to submit triangle strips to the PVR via the TA
    interface in the chip.

    An important side note about the PVR is that all primitive types
    must be submitted grouped together. If you have 10 polygons for each
    list type, then the PVR must receive them via the TA by list type,
    with a list delimiter in between.

    So there are two modes you can use here. The first mode allows you to
    submit data directly to the TA. Your data will be forwarded to the
    chip for processing as it is fed to the PVR module. If your data
    is easily sorted into the primitive types, then this is the fastest
    mode for submitting data.

    The second mode allows you to submit data via main-RAM vertex buffers,
    which will be queued until the proper primitive type is active. In this
    case, each piece of data is copied into the vertex buffer while the
    wrong list is activated, and when the proper list becomes activated,
    the data is all sent at once. Ideally this would be via DMA, right
    now it is by store queues. This has the advantage of allowing you to
    send data in any order and have the PVR functions resolve how it should
    get sent to the hardware, but it is slower.

    The nice thing is that any combination of these modes can be used. You
    can assign a vertex buffer for any list, and it will be used to hold the
    incoming vertex data until the proper list has come up. Or if the proper
    list is already up, the data will be submitted directly. So if most of
    your polygons are opaque, and you only have a couple of translucents,
    you can set a small buffer to gather translucent data and then it will
    get sent when you do a pvr_end_scene().

    Thanks to Mikael Kalms for the idea for this API.

    \note
    Another somewhat subtle point that bears mentioning is that in the normal
    case (interrupts enabled) an interrupt handler will automatically take
    care of starting a frame rendering (after scene_finish()) and also
    flipping pages when appropriate. 
*/

/** \defgroup  pvr_vertex_dma   Vertex DMA
    \brief                      Use the DMA to transfer inactive lists to the PVR
    \ingroup                    pvr_scene_mgmt
*/

/** \brief   Is vertex DMA enabled?
    \ingroup pvr_vertex_dma
    
    \return                 Non-zero if vertex DMA was enabled at init time
*/
int pvr_vertex_dma_enabled(void);

/** \brief   Setup a vertex buffer for one of the list types.
    \ingroup pvr_list_mgmt

    If the specified list type already has a vertex buffer, it will be replaced
    by the new one. 

    \note
    Each buffer should actually be twice as long as what you will need to hold
    two frames worth of data).

    \warning
    You should generally not try to do this at any time besides before a frame
    is begun, or Bad Things May Happen.

    \param  list            The primitive list to set the buffer for.
    \param  buffer          The location of the buffer in main RAM. This must be
                            aligned to a 32-byte boundary.
    \param  len             The length of the buffer. This must be a multiple of
                            64, and must be at least 128 (even if you're not
                            using the list).
    
    \return                 The old buffer location (if any)
*/
void *pvr_set_vertbuf(pvr_list_t list, void *buffer, size_t len);

/** \brief   Retrieve a pointer to the current output location in the DMA buffer
             for the requested list.
    \ingroup pvr_vertex_dma

    Vertex DMA must globally be enabled for this to work. Data may be added to
    this buffer by the user program directly; however, make sure to call
    pvr_vertbuf_written() to notify the system of any such changes.

    \param  list            The primitive list to get the buffer for.
    
    \return                 The tail of that list's buffer.
*/
void *pvr_vertbuf_tail(pvr_list_t list);

/** \brief   Notify the PVR system that data have been written into the output
             buffer for the given list.
    \ingroup pvr_vertex_dma

    This should always be done after writing data directly to these buffers or
    it will get overwritten by other data.

    \param  list            The primitive list that was modified.
    \param  amt             Number of bytes written. Must be a multiple of 32.
*/
void pvr_vertbuf_written(pvr_list_t list, size_t amt);

/** \brief   Set the translucent polygon sort mode for the next frame.
    \ingroup pvr_scene_mgmt

    This function sets the translucent polygon sort mode for the next frame of
    output, potentially switching between autosort and presort mode.

    For most programs, you'll probably want to set this at initialization time
    (with the autosort_disabled field in the pvr_init_params_t structure) and
    not mess with it per-frame. It is recommended that if you do use this
    function to change the mode that you should set it each frame to ensure that
    the mode is set properly.

    \param  presort         Set to true to set the presort mode for translucent
                            polygons, set to false to use autosort mode.
*/
void pvr_set_presort_mode(bool presort);

/** \brief   Begin collecting data for a frame of 3D output to the off-screen
             frame buffer.
    \ingroup pvr_scene_mgmt

    You must call this function (or pvr_scene_begin_txr()) for ever frame of
    output.
*/
void pvr_scene_begin(void);

/** \brief   Begin collecting data for a frame of 3D output to the specified
             texture.
    \ingroup pvr_scene_mgmt

    This function currently only supports outputting at the same size as the
    actual screen. Thus, make sure rx and ry are at least large enough for that.
    For a 640x480 output, rx will generally be 1024 on input and ry 512, as
    these are the smallest values that are powers of two and will hold the full
    screen sized output.

    \param  txr             The texture to render to.
    \param  rx              Width of the texture buffer (in pixels).
    \param  ry              Height of the texture buffer (in pixels).
*/
void pvr_scene_begin_txr(pvr_ptr_t txr, uint32_t *rx, uint32_t *ry);


/** \defgroup pvr_list_mgmt Polygon Lists
    \brief                  PVR API for managing list submission
    \ingroup                pvr_scene_mgmt
*/

/** \brief   Begin collecting data for the given list type.
    \ingroup pvr_list_mgmt

    Lists do not have to be submitted in any particular order, but all types of
    a list must be submitted at once (unless vertex DMA mode is enabled).

    Note that there is no need to call this function in DMA mode unless you want
    to make use of pvr_prim() for compatibility. This function will
    automatically call pvr_list_finish() if a list is already opened before
    opening the new list.

    \param  list            The list to open.
    \retval 0               On success.
    \retval -1              If the specified list has already been closed.
*/
int pvr_list_begin(pvr_list_t list);            

/** \brief   End collecting data for the current list type.
    \ingroup pvr_list_mgmt

    Lists can never be opened again within a single frame once they have been
    closed. Thus submitting a primitive that belongs in a closed list is
    considered an error. Closing a list that is already closed is also an error.

    Note that if you open a list but do not submit any primitives, a blank one
    will be submitted to satisfy the hardware. If vertex DMA mode is enabled,
    then this simply sets the current list pointer to no list, and none of the
    above restrictions apply.

    \retval 0               On success.
    \retval -1              On error.
*/
int pvr_list_finish(void);

/** \brief   Submit a primitive of the current list type.
    \ingroup pvr_list_mgmt

    Note that any values submitted in this fashion will go directly to the
    hardware without any sort of buffering, and submitting a primitive of the
    wrong type will quite likely ruin your scene. Note that this also will not
    work if you haven't begun any list types (i.e., all data is queued). If DMA
    is enabled, the primitive will be appended to the end of the currently
    selected list's buffer.

    \warning
    \p data must be 32-byte aligned!

    \param  data            The primitive to submit.
    \param  size            The length of the primitive, in bytes. Must be a
                            multiple of 32.
    
    \retval 0               On success.
    \retval -1              On error.
*/
int pvr_prim(const void *data, size_t size);

/** \defgroup pvr_direct  Direct Rendering
    \brief                API for using direct rendering with the PVR
    \ingroup              pvr_scene_mgmt

    @{
*/

/** \brief   Direct Rendering state variable type. */
typedef uint32_t pvr_dr_state_t;

/** \brief   Initialize a state variable for Direct Rendering.

    Store Queues are used.

    \param  vtx_buf_ptr     A variable of type pvr_dr_state_t to init.
*/
void pvr_dr_init(pvr_dr_state_t *vtx_buf_ptr);

/** \brief   Obtain the target address for Direct Rendering.

    \param  vtx_buf_ptr     State variable for Direct Rendering. Should be of
                            type pvr_dr_state_t, and must have been initialized
                            previously in the scene with pvr_dr_init().
    
    \return                 A write-only destination address where a primitive
                            should be written to get ready to submit it to the
                            TA in DR mode.
*/
#define pvr_dr_target(vtx_buf_ptr) \
    ({ (vtx_buf_ptr) ^= 32; \
        (pvr_vertex_t *)(MEM_AREA_SQ_BASE | (vtx_buf_ptr)); \
    })

/** \brief   Commit a primitive written into the Direct Rendering target address.

    \param  addr            The address returned by pvr_dr_target(), after you
                            have written the primitive to it.
*/
#define pvr_dr_commit(addr) sq_flush(addr)

/** \brief  Finish work with Direct Rendering.

    Called atomatically in pvr_scene_finish().
    Use it manually if you want to release Store Queues earlier.

*/
void pvr_dr_finish(void);

/** \brief  Upload a 32-byte payload to the Tile Accelerator

    Upload the given payload to the Tile Accelerator. The difference with the
    Direct Rendering approach above is that the Store Queues are not used, and
    therefore can be used for anything else.

    \param  data            A pointer to the 32-byte payload.
                            The pointer must be aligned to 8 bytes.
*/
void pvr_send_to_ta(void *data);

/** @} */

/** \brief   Submit a primitive of the given list type.
    \ingroup pvr_list_mgmt

    Data will be queued in a vertex buffer, thus one must be available for the
    list specified (will be asserted by the code).

    \param  list            The list to submit to.
    \param  data            The primitive to submit.
    \param  size            The size of the primitive in bytes. This must be a
                            multiple of 32.
    
    \retval 0               On success.
    \retval -1              On error.
*/
int pvr_list_prim(pvr_list_t list, const void *data, size_t size);

/** \brief   Flush the buffered data of the given list type to the TA.
    \ingroup pvr_list_mgmt

    This function is currently not implemented, and calling it will result in an
    assertion failure. It is intended to be used later in a "hybrid" mode where
    both direct and DMA TA submission is possible.

    \param  list            The list to flush.
    
    \retval -1              On error (it is not possible to succeed).
*/
int pvr_list_flush(pvr_list_t list);

/** \brief   Call this after you have finished submitting all data for a frame.
    \ingroup pvr_scene_mgmt

    Once this has been called, you can not submit any more data until one of the
    pvr_scene_begin() or pvr_scene_begin_txr() functions is called again.

    \retval 0               On success.
    \retval -1              On error (no scene started).
*/
int pvr_scene_finish(void);

/** \brief   Block the caller until the PVR system is ready for another frame to
             be submitted.
    \ingroup pvr_scene_mgmt

    The PVR system allocates enough space for two frames: one in data collection
    mode, and another in rendering mode. If a frame is currently rendering, and
    another frame has already been closed, then the caller cannot do anything
    else until the rendering frame completes. Note also that the new frame
    cannot be activated except during a vertical blanking period, so this
    essentially waits until a rendered frame is complete and a vertical blank
    happens.

    \retval 0               On success. A new scene can be started now.
    \retval -1              On error. Something is probably very wrong...
*/
int pvr_wait_ready(void);

/** \brief   Check if the PVR system is ready for another frame to be submitted.
    \ingroup pvr_scene_mgmt

    \retval 0               If the PVR is ready for a new scene. You must call
                            pvr_wait_ready() afterwards, before starting a new
                            scene.
    \retval -1              If the PVR is not ready for a new scene yet.
*/
int pvr_check_ready(void);

/** \brief   Block the caller until the PVR has finished rendering the previous
             frame.
    \ingroup pvr_scene_mgmt

    This function can be used to wait until the PVR is done rendering a previous
    scene. This can be useful for instance to make sure that the PVR is done
    using textures that have to be updated, before updating those.

    \retval 0               On success.
    \retval -1              On error. Something is probably very wrong...
*/
int pvr_wait_render_done(void);


/* Primitive handling ************************************************/

/** \defgroup pvr_primitives_compilation Compilation
    \brief                               API for compiling primitive contexts
                                         into headers
    \ingroup pvr_ctx
*/

/** \brief   Compile a polygon context into a polygon header.
    \ingroup pvr_primitives_compilation

    This function compiles a pvr_poly_cxt_t into the form needed by the hardware
    for rendering. This is for use with normal polygon headers.

    \param  dst             Where to store the compiled header.
    \param  src             The context to compile.
*/
void pvr_poly_compile(pvr_poly_hdr_t *dst, const pvr_poly_cxt_t *src);

/** \defgroup pvr_ctx_init     Initialization
    \brief                     Functions for initializing PVR polygon contexts
    \ingroup                   pvr_ctx
*/

/** \brief   Fill in a polygon context for non-textured polygons.
    \ingroup pvr_ctx_init

    This function fills in a pvr_poly_cxt_t with default parameters appropriate
    for rendering a non-textured polygon in the given list.

    \param  dst             Where to store the polygon context.
    \param  list            The primitive list to be used.
*/
void pvr_poly_cxt_col(pvr_poly_cxt_t *dst, pvr_list_t list);

/** \brief   Fill in a polygon context for a textured polygon.
    \ingroup pvr_ctx_init

    This function fills in a pvr_poly_cxt_t with default parameters appropriate
    for rendering a textured polygon in the given list.

    \param  dst             Where to store the polygon context.
    \param  list            The primitive list to be used.
    \param  textureformat   The format of the texture used.
    \param  tw              The width of the texture, in pixels.
    \param  th              The height of the texture, in pixels.
    \param  textureaddr     A pointer to the texture.
    \param  filtering       The type of filtering to use.

    \see    pvr_txr_fmts
    \see    pvr_filter_modes
*/
void pvr_poly_cxt_txr(pvr_poly_cxt_t *dst, pvr_list_t list,
                      int textureformat, int tw, int th, pvr_ptr_t textureaddr,
                      int filtering);

/** \brief   Compile a sprite context into a sprite header.
    \ingroup pvr_primitives_compilation

    This function compiles a pvr_sprite_cxt_t into the form needed by the
    hardware for rendering. This is for use with sprite headers.

    \param  dst             Where to store the compiled header.
    \param  src             The context to compile.
*/
void pvr_sprite_compile(pvr_sprite_hdr_t *dst,
                        const pvr_sprite_cxt_t *src);

/** \brief   Fill in a sprite context for non-textured sprites.
    \ingroup pvr_ctx_init

    This function fills in a pvr_sprite_cxt_t with default parameters
    appropriate for rendering a non-textured sprite in the given list.

    \param  dst             Where to store the sprite context.
    \param  list            The primitive list to be used.
*/
void pvr_sprite_cxt_col(pvr_sprite_cxt_t *dst, pvr_list_t list);

/** \brief   Fill in a sprite context for a textured sprite.
    \ingroup pvr_ctx_init

    This function fills in a pvr_sprite_cxt_t with default parameters
    appropriate for rendering a textured sprite in the given list.

    \param  dst             Where to store the sprite context.
    \param  list            The primitive list to be used.
    \param  textureformat   The format of the texture used.
    \param  tw              The width of the texture, in pixels.
    \param  th              The height of the texture, in pixels.
    \param  textureaddr     A pointer to the texture.
    \param  filtering       The type of filtering to use.

    \see    pvr_txr_fmts
    \see    pvr_filter_modes
*/
void pvr_sprite_cxt_txr(pvr_sprite_cxt_t *dst, pvr_list_t list,
                        int textureformat, int tw, int th, pvr_ptr_t textureaddr,
                        int filtering);

/** \brief   Create a modifier volume header.
    \ingroup pvr_primitives_compilation

    This function fills in a modifier volume header with the parameters
    specified. Note that unlike for polygons and sprites, there is no context
    step for modifiers.

    \param  dst             Where to store the modifier header.
    \param  list            The primitive list to be used.
    \param  mode            The mode for this modifier.
    \param  cull            The culling mode to use.

    \see    pvr_mod_modes
    \see    pvr_cull_modes
*/
void pvr_mod_compile(pvr_mod_hdr_t *dst, pvr_list_t list, uint32_t mode,
                     uint32_t cull);

/** \brief   Compile a polygon context into a polygon header that is affected by
             modifier volumes.
    \ingroup pvr_primitives_compilation

    This function works pretty similarly to pvr_poly_compile(), but compiles
    into the header type that is affected by a modifier volume. The context
    should have been created with either pvr_poly_cxt_col_mod() or
    pvr_poly_cxt_txr_mod().

    \param  dst             Where to store the compiled header.
    \param  src             The context to compile.
*/
void pvr_poly_mod_compile(pvr_poly_mod_hdr_t *dst, const pvr_poly_cxt_t *src);

/** \brief   Fill in a polygon context for non-textured polygons affected by a
             modifier volume.
    \ingroup pvr_ctx_init

    This function fills in a pvr_poly_cxt_t with default parameters appropriate
    for rendering a non-textured polygon in the given list that will be affected
    by modifier volumes.

    \param  dst             Where to store the polygon context.
    \param  list            The primitive list to be used.
*/
void pvr_poly_cxt_col_mod(pvr_poly_cxt_t *dst, pvr_list_t list);

/** \brief   Fill in a polygon context for a textured polygon affected by
             modifier volumes.
    \ingroup pvr_ctx_init

    This function fills in a pvr_poly_cxt_t with default parameters appropriate
    for rendering a textured polygon in the given list and being affected by
    modifier volumes.

    \param  dst             Where to store the polygon context.
    \param  list            The primitive list to be used.
    \param  textureformat   The format of the texture used (outside).
    \param  tw              The width of the texture, in pixels (outside).
    \param  th              The height of the texture, in pixels (outside).
    \param  textureaddr     A pointer to the texture (outside).
    \param  filtering       The type of filtering to use (outside).
    \param  textureformat2  The format of the texture used (inside).
    \param  tw2             The width of the texture, in pixels (inside).
    \param  th2             The height of the texture, in pixels (inside).
    \param  textureaddr2    A pointer to the texture (inside).
    \param  filtering2      The type of filtering to use (inside).

    \see    pvr_txr_fmts
    \see    pvr_filter_modes
*/
void pvr_poly_cxt_txr_mod(pvr_poly_cxt_t *dst, pvr_list_t list,
                          int textureformat, int tw, int th,
                          pvr_ptr_t textureaddr, int filtering,
                          int textureformat2, int tw2, int th2,
                          pvr_ptr_t textureaddr2, int filtering2);

/** \brief   Get a pointer to the front buffer.
    \ingroup pvr_txr_mgmt

    This function can be used to retrieve a pointer to the front buffer, aka.
    the last fully rendered buffer that is either being displayed right now,
    or is queued to be displayed.

    Note that the frame buffers lie in 32-bit memory, while textures lie in
    64-bit memory. The address returned will point to 64-bit memory, but the
    front buffer cannot be used directly as a regular texture.

    \return                 A pointer to the front buffer.
*/
pvr_ptr_t pvr_get_front_buffer(void);

/*********************************************************************/

#include "pvr/pvr_regs.h"
#include "pvr/pvr_dma.h"
#include "pvr/pvr_fog.h"
#include "pvr/pvr_pal.h"
#include "pvr/pvr_txr.h"

__END_DECLS

#endif
