/* KallistiOS ##version##
   dc/pvr/pvr_legacy.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2014 Lawrence Sebald
   Copyright (C) 2023 Ruslan Rostovtsev
   Copyright (C) 2024 Falco Girgis
*/

/** \file    dc/pvr/pvr_legacy.h
    \brief   All deprecated PVR API Constants
    \ingroup pvr_legacy
    \author Megan Potter
    \author Roger Cattermole
    \author Paul Boese
    \author Brian Paul
    \author Lawrence Sebald
    \author Benoit Miller
    \author Ruslan Rostovtsev
    \author Falco Girgis
*/

#ifndef __DC_PVR_PVR_LEGACY_H
#define __DC_PVR_PVR_LEGACY_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \defgroup pvr_legacy    Legacy Constants
    \ingroup  pvr
    \brief    Deprecated and legacy defines for constants.
    \deprecated
    These were essentially pointless code-bloat and their respective fields
    have since been refactored in the API to use regular C boolean types.
    \note
    This file remains only for backwards compatibility reasons.
    @{
*/

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

/** \defgroup pvr_mod_types         Types
    \brief                          Modifier volume types for PowerVR primitive contexts
    \ingroup                        pvr_ctx_modvol
    @{
*/
#define PVR_MODIFIER_CHEAP_SHADOW   0
#define PVR_MODIFIER_NORMAL         1
/** @} */

/** \defgroup pvr_blend             Blending
    \brief                          Blending attributes for PVR primitive contexts
    \ingroup                        pvr_ctx_attrib
*/

/** \defgroup pvr_blend_switch      Blending Toggle
    \brief                          Enable or Disable Blending.
    \ingroup                        pvr_blend

    @{
*/
#define PVR_BLEND_DISABLE       0   /**< \brief Disable blending */
#define PVR_BLEND_ENABLE        1   /**< \brief Enable blending */
/** @} */

/** \defgroup pvr_uv_fmts           U/V Data Format
    \brief                          U/V data format for PVR textures
    \ingroup                        pvr_ctx_texture
    @{
*/
#define PVR_UVFMT_32BIT         0   /**< \brief 32-bit floating point U/V */
#define PVR_UVFMT_16BIT         1   /**< \brief 16-bit floating point U/V */
/** @} */

/** \defgroup pvr_mod_switch        Toggle
    \brief                          Enable or Disable Modifier Effects
    \ingroup                        pvr_ctx_modvol
    @{
*/
#define PVR_MODIFIER_DISABLE    0   /**< \brief Disable modifier effects */
#define PVR_MODIFIER_ENABLE     1   /**< \brief Enable modifier effects */
/** @} */

/** \defgroup pvr_depth_switch      Write Toggle
    \brief                          Enable or Disable Depth Writes.
    \ingroup                        pvr_ctx_depth
    @{
*/
#define PVR_DEPTHWRITE_ENABLE   0   /**< \brief Update the Z value */
#define PVR_DEPTHWRITE_DISABLE  1   /**< \brief Do not update the Z value */
/** @} */

/** \defgroup pvr_txr_switch        Toggle
    \brief                          Enable or Disable Texturing on Polygons.
    \ingroup                        pvr_ctx_texture

    @{
*/
#define PVR_TEXTURE_DISABLE     0   /**< \brief Disable texturing */
#define PVR_TEXTURE_ENABLE      1   /**< \brief Enable texturing */
/** @} */

/** \defgroup pvr_mip_switch        Mipmap Toggle
    \brief                          Enable or Disable Mipmap Processing
    \ingroup                        pvr_ctx_texture

    @{
*/
#define PVR_MIPMAP_DISABLE      0   /**< \brief Disable mipmap processing */
#define PVR_MIPMAP_ENABLE       1   /**< \brief Enable mipmap processing */
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

/** \defgroup pvr_bitmasks_legacy   Constants and Masks
    \brief                          Legacy polygon header constants and masks
    \deprecated                     Replaced by \ref pvr_bitmasks
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
#define PVR_TA_CMD_MODIFIER_MASK        (1 << PVR_TA_CMD_MODIFIER_SHIFT)

#define PVR_TA_CMD_MODIFIERMODE_SHIFT   6
#define PVR_TA_CMD_MODIFIERMODE_MASK    (1 << PVR_TA_CMD_MODIFIERMODE_SHIFT)

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

#define PVR_TA_PM2_FILTER_SHIFT         13
#define PVR_TA_PM2_FILTER_MASK          (3 << PVR_TA_PM2_FILTER_SHIFT)

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

typedef uint32_t pvr_dr_state_t;

__depr("pvr_dr_init is not useful anymore")
static inline void pvr_dr_init(pvr_dr_state_t *vtx_buf_ptr) {
    (void)vtx_buf_ptr;
}

__depr("pvr_dr_finish is not useful anymore")
static inline void pvr_dr_finish(void) {
}

/** @} */

__END_DECLS

#endif  /* __DC_PVR_PVR_LEGACY_H */