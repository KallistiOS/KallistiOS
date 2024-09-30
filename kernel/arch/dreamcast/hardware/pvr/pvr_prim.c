/* KallistiOS ##version##

   pvr_prim.c
   Copyright (C) 2002 Megan Potter

 */

#include <assert.h>
#include <string.h>
#include <dc/pvr.h>
#include "pvr_internal.h"

/*

   Primitive handling

   These functions help you prepare primitives for loading into the
   PVR for scene processing.

*/

/* Compile a polygon context into a polygon header */
void pvr_poly_compile(pvr_poly_hdr_t *dst, const pvr_poly_cxt_t *src) {
    int u, v;
    uint32_t txr_base;
    /* Temporary variables we can read-write-modify, since we cannot do so from
       within the SQs, and we want to be able to compile this header from a PVR
       DR API submission target. */
    uint32_t cmd, mode[3];

    /* Basically we just take each parameter, clip it, shift it
       into place, and OR it into the final result. */

    /* The base values for CMD */
    cmd = PVR_CMD_POLYHDR;

    if(src->txr.enable == PVR_TEXTURE_ENABLE)
        cmd |= 8;

    /* Or in the list type, shading type, color and UV formats */
    cmd |= (src->list_type << PVR_TA_CMD_TYPE_SHIFT) & PVR_TA_CMD_TYPE_MASK;
    cmd |= (src->fmt.color << PVR_TA_CMD_CLRFMT_SHIFT) & PVR_TA_CMD_CLRFMT_MASK;
    cmd |= (src->gen.shading << PVR_TA_CMD_SHADE_SHIFT) & PVR_TA_CMD_SHADE_MASK;
    cmd |= (src->fmt.uv << PVR_TA_CMD_UVFMT_SHIFT) & PVR_TA_CMD_UVFMT_MASK;
    cmd |= (src->gen.clip_mode << PVR_TA_CMD_USERCLIP_SHIFT) & PVR_TA_CMD_USERCLIP_MASK;
    cmd |= (src->fmt.modifier << PVR_TA_CMD_MODIFIER_SHIFT) & PVR_TA_CMD_MODIFIER_MASK;
    cmd |= (src->gen.modifier_mode << PVR_TA_CMD_MODIFIERMODE_SHIFT) & PVR_TA_CMD_MODIFIERMODE_MASK;
    cmd |= (src->gen.specular << PVR_TA_CMD_SPECULAR_SHIFT) & PVR_TA_CMD_SPECULAR_MASK;

    dst->cmd = cmd;

    /* Polygon mode 1 */
    mode[0]  = (src->depth.comparison << PVR_TA_PM1_DEPTHCMP_SHIFT) & PVR_TA_PM1_DEPTHCMP_MASK;
    mode[0] |= (src->gen.culling << PVR_TA_PM1_CULLING_SHIFT) & PVR_TA_PM1_CULLING_MASK;
    mode[0] |= (src->depth.write << PVR_TA_PM1_DEPTHWRITE_SHIFT) & PVR_TA_PM1_DEPTHWRITE_MASK;
    mode[0] |= (src->txr.enable << PVR_TA_PM1_TXRENABLE_SHIFT) & PVR_TA_PM1_TXRENABLE_MASK;

    dst->mode1 = mode[0];

    /* Polygon mode 2 */
    mode[1]  = (src->blend.src << PVR_TA_PM2_SRCBLEND_SHIFT) & PVR_TA_PM2_SRCBLEND_MASK;
    mode[1] |= (src->blend.dst << PVR_TA_PM2_DSTBLEND_SHIFT) & PVR_TA_PM2_DSTBLEND_MASK;
    mode[1] |= (src->blend.src_enable << PVR_TA_PM2_SRCENABLE_SHIFT) & PVR_TA_PM2_SRCENABLE_MASK;
    mode[1] |= (src->blend.dst_enable << PVR_TA_PM2_DSTENABLE_SHIFT) & PVR_TA_PM2_DSTENABLE_MASK;
    mode[1] |= (src->gen.fog_type << PVR_TA_PM2_FOG_SHIFT) & PVR_TA_PM2_FOG_MASK;
    mode[1] |= (src->gen.color_clamp << PVR_TA_PM2_CLAMP_SHIFT) & PVR_TA_PM2_CLAMP_MASK;
    mode[1] |= (src->gen.alpha << PVR_TA_PM2_ALPHA_SHIFT) & PVR_TA_PM2_ALPHA_MASK;

    if(src->txr.enable == PVR_TEXTURE_DISABLE) {
        dst->mode2 = mode[1];
        mode[2] = 0;
        dst->mode3 = mode[2];
    }
    else {
        mode[1] |= (src->txr.alpha << PVR_TA_PM2_TXRALPHA_SHIFT) & PVR_TA_PM2_TXRALPHA_MASK;
        mode[1] |= (src->txr.uv_flip << PVR_TA_PM2_UVFLIP_SHIFT) & PVR_TA_PM2_UVFLIP_MASK;
        mode[1] |= (src->txr.uv_clamp << PVR_TA_PM2_UVCLAMP_SHIFT) & PVR_TA_PM2_UVCLAMP_MASK;
        mode[1] |= (src->txr.filter << PVR_TA_PM2_FILTER_SHIFT) & PVR_TA_PM2_FILTER_MASK;
        mode[1] |= (src->txr.mipmap_bias << PVR_TA_PM2_MIPBIAS_SHIFT) & PVR_TA_PM2_MIPBIAS_MASK;
        mode[1] |= (src->txr.env << PVR_TA_PM2_TXRENV_SHIFT) & PVR_TA_PM2_TXRENV_MASK;

        switch(src->txr.width) {
            case 8:
                u = 0;
                break;
            case 16:
                u = 1;
                break;
            case 32:
                u = 2;
                break;
            case 64:
                u = 3;
                break;
            case 128:
                u = 4;
                break;
            case 256:
                u = 5;
                break;
            case 512:
                u = 6;
                break;
            case 1024:
                u = 7;
                break;
            default:
                assert_msg(0, "Invalid texture U size");
                u = 0;
                break;
        }

        switch(src->txr.height) {
            case 8:
                v = 0;
                break;
            case 16:
                v = 1;
                break;
            case 32:
                v = 2;
                break;
            case 64:
                v = 3;
                break;
            case 128:
                v = 4;
                break;
            case 256:
                v = 5;
                break;
            case 512:
                v = 6;
                break;
            case 1024:
                v = 7;
                break;
            default:
                assert_msg(0, "Invalid texture V size");
                v = 0;
                break;
        }

        mode[1] |= (u << PVR_TA_PM2_USIZE_SHIFT) & PVR_TA_PM2_USIZE_MASK;
        mode[1] |= (v << PVR_TA_PM2_VSIZE_SHIFT) & PVR_TA_PM2_VSIZE_MASK;

        dst->mode2 = mode[1];

        /* Polygon mode 3 */
        mode[2]  = (src->txr.mipmap << PVR_TA_PM3_MIPMAP_SHIFT) & PVR_TA_PM3_MIPMAP_MASK;
        mode[2] |= (src->txr.format << PVR_TA_PM3_TXRFMT_SHIFT) & PVR_TA_PM3_TXRFMT_MASK;

        /* Convert the texture address */
        txr_base = (uint32_t)src->txr.base;
        txr_base = (txr_base & 0x00fffff8) >> 3;
        mode[2] |= txr_base;

        dst->mode3 = mode[2];
    }

    if(src->fmt.modifier && src->gen.modifier_mode) {
        /* If we're affected by a modifier volume, silently promote the header
           to the one that is affected by a modifier volume. */
        dst->d1 = mode[1];
        dst->d2 = mode[2];
    }
    else {
        dst->d1 = 0xffffffff;
        dst->d2 = 0xffffffff;
    }

    dst->d3 = 0xffffffff;
    dst->d4 = 0xffffffff;
}

/* Create a colored polygon context with parameters similar to
   the old "ta" function `ta_poly_hdr_col' */
void pvr_poly_cxt_col(pvr_poly_cxt_t *dst, pvr_list_t list) {
    int alpha;

    /* Start off blank */
    memset(dst, 0, sizeof(pvr_poly_cxt_t));

    /* Fill in a few values */
    dst->list_type = list;
    alpha = list > PVR_LIST_OP_MOD;
    dst->fmt.color = PVR_CLRFMT_ARGBPACKED;
    dst->fmt.uv = PVR_UVFMT_32BIT;
    dst->gen.shading = PVR_SHADE_GOURAUD;
    dst->depth.comparison = PVR_DEPTHCMP_GREATER;
    dst->depth.write = PVR_DEPTHWRITE_ENABLE;
    dst->gen.culling = PVR_CULLING_CCW;
    dst->txr.enable = PVR_TEXTURE_DISABLE;

    if(!alpha) {
        dst->gen.alpha = PVR_ALPHA_DISABLE;
        dst->blend.src = PVR_BLEND_ONE;
        dst->blend.dst = PVR_BLEND_ZERO;
    }
    else {
        dst->gen.alpha = PVR_ALPHA_ENABLE;
        dst->blend.src = PVR_BLEND_SRCALPHA;
        dst->blend.dst = PVR_BLEND_INVSRCALPHA;
    }

    dst->blend.src_enable = PVR_BLEND_DISABLE;
    dst->blend.dst_enable = PVR_BLEND_DISABLE;
    dst->gen.fog_type = PVR_FOG_DISABLE;
    dst->gen.color_clamp = PVR_CLRCLAMP_DISABLE;
}

/* Create a textured polygon context with parameters similar to
   the old "ta" function `ta_poly_hdr_txr' */
void pvr_poly_cxt_txr(pvr_poly_cxt_t *dst, pvr_list_t list,
                      int textureformat, int tw, int th, pvr_ptr_t textureaddr,
                      int filtering) {
    int alpha;

    /* Start off blank */
    memset(dst, 0, sizeof(pvr_poly_cxt_t));

    /* Fill in a few values */
    dst->list_type = list;
    alpha = list > PVR_LIST_OP_MOD;
    dst->fmt.color = PVR_CLRFMT_ARGBPACKED;
    dst->fmt.uv = PVR_UVFMT_32BIT;
    dst->gen.shading = PVR_SHADE_GOURAUD;
    dst->depth.comparison = PVR_DEPTHCMP_GREATER;
    dst->depth.write = PVR_DEPTHWRITE_ENABLE;
    dst->gen.culling = PVR_CULLING_CCW;
    dst->txr.enable = PVR_TEXTURE_ENABLE;

    if(!alpha) {
        dst->gen.alpha = PVR_ALPHA_DISABLE;
        dst->txr.alpha = PVR_TXRALPHA_ENABLE;
        dst->blend.src = PVR_BLEND_ONE;
        dst->blend.dst = PVR_BLEND_ZERO;
        dst->txr.env = PVR_TXRENV_MODULATE;
    }
    else {
        dst->gen.alpha = PVR_ALPHA_ENABLE;
        dst->txr.alpha = PVR_TXRALPHA_ENABLE;
        dst->blend.src = PVR_BLEND_SRCALPHA;
        dst->blend.dst = PVR_BLEND_INVSRCALPHA;
        dst->txr.env = PVR_TXRENV_MODULATEALPHA;
    }

    dst->blend.src_enable = PVR_BLEND_DISABLE;
    dst->blend.dst_enable = PVR_BLEND_DISABLE;
    dst->gen.fog_type = PVR_FOG_DISABLE;
    dst->gen.color_clamp = PVR_CLRCLAMP_DISABLE;
    dst->txr.uv_flip = PVR_UVFLIP_NONE;
    dst->txr.uv_clamp = PVR_UVCLAMP_NONE;
    dst->txr.filter = filtering;
    dst->txr.mipmap_bias = PVR_MIPBIAS_NORMAL;
    dst->txr.width = tw;
    dst->txr.height = th;
    dst->txr.base = textureaddr;
    dst->txr.format = textureformat;
}

/* Create an untextured sprite context. */
void pvr_sprite_cxt_col(pvr_sprite_cxt_t *dst, pvr_list_t list) {
    int alpha;

    /* Start off blank */
    memset(dst, 0, sizeof(pvr_sprite_cxt_t));

    /* Fill in a few values */
    dst->list_type = list;
    alpha = list > PVR_LIST_OP_MOD;
    dst->depth.comparison = PVR_DEPTHCMP_GREATER;
    dst->depth.write = PVR_DEPTHWRITE_ENABLE;
    dst->gen.culling = PVR_CULLING_CCW;
    dst->txr.enable = PVR_TEXTURE_DISABLE;

    if(!alpha) {
        dst->gen.alpha = PVR_ALPHA_DISABLE;
        dst->blend.src = PVR_BLEND_ONE;
        dst->blend.dst = PVR_BLEND_ZERO;
    }
    else {
        dst->gen.alpha = PVR_ALPHA_ENABLE;
        dst->blend.src = PVR_BLEND_SRCALPHA;
        dst->blend.dst = PVR_BLEND_INVSRCALPHA;
    }

    dst->blend.src_enable = PVR_BLEND_DISABLE;
    dst->blend.dst_enable = PVR_BLEND_DISABLE;
    dst->gen.fog_type = PVR_FOG_DISABLE;
    dst->gen.color_clamp = PVR_CLRCLAMP_DISABLE;
}

/* Create a textured sprite context. */
void pvr_sprite_cxt_txr(pvr_sprite_cxt_t *dst, pvr_list_t list,
                        int textureformat, int tw, int th, pvr_ptr_t textureaddr,
                        int filtering) {
    int alpha;

    /* Start off blank */
    memset(dst, 0, sizeof(pvr_sprite_cxt_t));

    /* Fill in a few values */
    dst->list_type = list;
    alpha = list > PVR_LIST_OP_MOD;
    dst->depth.comparison = PVR_DEPTHCMP_GREATER;
    dst->depth.write = PVR_DEPTHWRITE_ENABLE;
    dst->gen.culling = PVR_CULLING_CCW;

    if(!alpha) {
        dst->gen.alpha = PVR_ALPHA_DISABLE;
        dst->txr.alpha = PVR_TXRALPHA_ENABLE;
        dst->blend.src = PVR_BLEND_ONE;
        dst->blend.dst = PVR_BLEND_ZERO;
        dst->txr.env = PVR_TXRENV_MODULATE;
    }
    else {
        dst->gen.alpha = PVR_ALPHA_ENABLE;
        dst->txr.alpha = PVR_TXRALPHA_ENABLE;
        dst->blend.src = PVR_BLEND_SRCALPHA;
        dst->blend.dst = PVR_BLEND_INVSRCALPHA;
        dst->txr.env = PVR_TXRENV_MODULATEALPHA;
    }

    dst->blend.src_enable = PVR_BLEND_DISABLE;
    dst->blend.dst_enable = PVR_BLEND_DISABLE;
    dst->gen.fog_type = PVR_FOG_DISABLE;
    dst->gen.color_clamp = PVR_CLRCLAMP_DISABLE;
    dst->txr.enable = PVR_TEXTURE_ENABLE;
    dst->txr.uv_flip = PVR_UVFLIP_NONE;
    dst->txr.uv_clamp = PVR_UVCLAMP_NONE;
    dst->txr.filter = filtering;
    dst->txr.mipmap_bias = PVR_MIPBIAS_NORMAL;
    dst->txr.width = tw;
    dst->txr.height = th;
    dst->txr.base = textureaddr;
    dst->txr.format = textureformat;
}

void pvr_sprite_compile(pvr_sprite_hdr_t *dst, const pvr_sprite_cxt_t *src) {
    int u, v;
    uint32_t txr_base;
    uint32_t cmd, mode[3];

    /* Basically we just take each parameter, clip it, shift it
       into place, and OR it into the final result. */

    /* The base values for CMD */
    cmd = PVR_CMD_SPRITE;

    if(src->txr.enable == PVR_TEXTURE_ENABLE)
        cmd |= 8;

    /* Or in the list type, clipping mode, and UV formats */
    cmd |= (src->list_type << PVR_TA_CMD_TYPE_SHIFT) & PVR_TA_CMD_TYPE_MASK;
    cmd |= (PVR_UVFMT_16BIT << PVR_TA_CMD_UVFMT_SHIFT) & PVR_TA_CMD_UVFMT_MASK;
    cmd |= (src->gen.clip_mode << PVR_TA_CMD_USERCLIP_SHIFT) & PVR_TA_CMD_USERCLIP_MASK;
    cmd |= (src->gen.specular << PVR_TA_CMD_SPECULAR_SHIFT) & PVR_TA_CMD_SPECULAR_MASK;

    dst->cmd = cmd;

    /* Polygon mode 1 */
    mode[0]  = (src->depth.comparison << PVR_TA_PM1_DEPTHCMP_SHIFT) & PVR_TA_PM1_DEPTHCMP_MASK;
    mode[0] |= (src->gen.culling << PVR_TA_PM1_CULLING_SHIFT) & PVR_TA_PM1_CULLING_MASK;
    mode[0] |= (src->depth.write << PVR_TA_PM1_DEPTHWRITE_SHIFT) & PVR_TA_PM1_DEPTHWRITE_MASK;
    mode[0] |= (src->txr.enable << PVR_TA_PM1_TXRENABLE_SHIFT) & PVR_TA_PM1_TXRENABLE_MASK;

    dst->mode1 = mode[0];

    /* Polygon mode 2 */
    mode[1]  = (src->blend.src << PVR_TA_PM2_SRCBLEND_SHIFT) & PVR_TA_PM2_SRCBLEND_MASK;
    mode[1] |= (src->blend.dst << PVR_TA_PM2_DSTBLEND_SHIFT) & PVR_TA_PM2_DSTBLEND_MASK;
    mode[1] |= (src->blend.src_enable << PVR_TA_PM2_SRCENABLE_SHIFT) & PVR_TA_PM2_SRCENABLE_MASK;
    mode[1] |= (src->blend.dst_enable << PVR_TA_PM2_DSTENABLE_SHIFT) & PVR_TA_PM2_DSTENABLE_MASK;
    mode[1] |= (src->gen.fog_type << PVR_TA_PM2_FOG_SHIFT) & PVR_TA_PM2_FOG_MASK;
    mode[1] |= (src->gen.color_clamp << PVR_TA_PM2_CLAMP_SHIFT) & PVR_TA_PM2_CLAMP_MASK;
    mode[1] |= (src->gen.alpha << PVR_TA_PM2_ALPHA_SHIFT) & PVR_TA_PM2_ALPHA_MASK;

    if(src->txr.enable == PVR_TEXTURE_DISABLE) {
        dst->mode2 = mode[1];
        dst->mode3 = 0;
    }
    else {
        mode[1] |= (src->txr.alpha << PVR_TA_PM2_TXRALPHA_SHIFT) & PVR_TA_PM2_TXRALPHA_MASK;
        mode[1] |= (src->txr.uv_flip << PVR_TA_PM2_UVFLIP_SHIFT) & PVR_TA_PM2_UVFLIP_MASK;
        mode[1] |= (src->txr.uv_clamp << PVR_TA_PM2_UVCLAMP_SHIFT) & PVR_TA_PM2_UVCLAMP_MASK;
        mode[1] |= (src->txr.filter << PVR_TA_PM2_FILTER_SHIFT) & PVR_TA_PM2_FILTER_MASK;
        mode[1] |= (src->txr.mipmap_bias << PVR_TA_PM2_MIPBIAS_SHIFT) & PVR_TA_PM2_MIPBIAS_MASK;
        mode[1] |= (src->txr.env << PVR_TA_PM2_TXRENV_SHIFT) & PVR_TA_PM2_TXRENV_MASK;

        switch(src->txr.width) {
            case 8:
                u = 0;
                break;
            case 16:
                u = 1;
                break;
            case 32:
                u = 2;
                break;
            case 64:
                u = 3;
                break;
            case 128:
                u = 4;
                break;
            case 256:
                u = 5;
                break;
            case 512:
                u = 6;
                break;
            case 1024:
                u = 7;
                break;
            default:
                assert_msg(0, "Invalid texture U size");
                u = 0;
                break;
        }

        switch(src->txr.height) {
            case 8:
                v = 0;
                break;
            case 16:
                v = 1;
                break;
            case 32:
                v = 2;
                break;
            case 64:
                v = 3;
                break;
            case 128:
                v = 4;
                break;
            case 256:
                v = 5;
                break;
            case 512:
                v = 6;
                break;
            case 1024:
                v = 7;
                break;
            default:
                assert_msg(0, "Invalid texture V size");
                v = 0;
                break;
        }

        mode[1] |= (u << PVR_TA_PM2_USIZE_SHIFT) & PVR_TA_PM2_USIZE_MASK;
        mode[1] |= (v << PVR_TA_PM2_VSIZE_SHIFT) & PVR_TA_PM2_VSIZE_MASK;

        dst->mode2 = mode[1];

        /* Polygon mode 3 */
        mode[2]  = (src->txr.mipmap << PVR_TA_PM3_MIPMAP_SHIFT) & PVR_TA_PM3_MIPMAP_MASK;
        mode[2] |= (src->txr.format << PVR_TA_PM3_TXRFMT_SHIFT) & PVR_TA_PM3_TXRFMT_MASK;

        txr_base = (uint32_t)src->txr.base;
        txr_base = (txr_base & 0x00fffff8) >> 3;
        mode[2] |= txr_base;

        dst->mode3 = mode[2];
    }

    dst->argb = 0xFFFFFFFF;
    dst->oargb = 0x00000000;
}

void pvr_mod_compile(pvr_mod_hdr_t *dst, pvr_list_t list, uint32 mode,
                     uint32 cull) {
    dst->cmd = PVR_CMD_MODIFIER;
    dst->cmd |= (list << PVR_TA_CMD_TYPE_SHIFT) & PVR_TA_CMD_TYPE_MASK;

    dst->mode1 = (mode << PVR_TA_PM1_MODIFIERINST_SHIFT) & PVR_TA_PM1_MODIFIERINST_MASK;
    dst->mode1 |= (cull << PVR_TA_PM1_CULLING_SHIFT) & PVR_TA_PM1_CULLING_MASK;

    dst->d1 = dst->d2 = dst->d3 = dst->d4 = dst->d5 = dst->d6 = 0;
}

/* Compile a polygon context into a polygon header that is affected by
   modifier volumes */
void pvr_poly_mod_compile(pvr_poly_mod_hdr_t *dst, const pvr_poly_cxt_t *src) {
    int u, v;
    uint32_t txr_base;
    uint32_t cmd, mode1, mode2[2], mode3[2];

    /* Basically we just take each parameter, clip it, shift it
       into place, and OR it into the final result. */

    /* The base values for CMD */
    cmd = PVR_CMD_POLYHDR;

    if(src->txr.enable == PVR_TEXTURE_ENABLE)
        cmd |= 8;

    /* Or in the list type, shading type, color and UV formats */
    cmd |= (src->list_type << PVR_TA_CMD_TYPE_SHIFT) & PVR_TA_CMD_TYPE_MASK;
    cmd |= (src->fmt.color << PVR_TA_CMD_CLRFMT_SHIFT) & PVR_TA_CMD_CLRFMT_MASK;
    cmd |= (src->gen.shading << PVR_TA_CMD_SHADE_SHIFT) & PVR_TA_CMD_SHADE_MASK;
    cmd |= (src->fmt.uv << PVR_TA_CMD_UVFMT_SHIFT) & PVR_TA_CMD_UVFMT_MASK;
    cmd |= (src->gen.clip_mode << PVR_TA_CMD_USERCLIP_SHIFT) & PVR_TA_CMD_USERCLIP_MASK;
    cmd |= (src->fmt.modifier << PVR_TA_CMD_MODIFIER_SHIFT) & PVR_TA_CMD_MODIFIER_MASK;
    cmd |= (src->gen.modifier_mode << PVR_TA_CMD_MODIFIERMODE_SHIFT) & PVR_TA_CMD_MODIFIERMODE_MASK;
    cmd |= (src->gen.specular << PVR_TA_CMD_SPECULAR_SHIFT) & PVR_TA_CMD_SPECULAR_MASK;

    dst->cmd = cmd;

    /* Polygon mode 1 */
    mode1  = (src->depth.comparison << PVR_TA_PM1_DEPTHCMP_SHIFT) & PVR_TA_PM1_DEPTHCMP_MASK;
    mode1 |= (src->gen.culling << PVR_TA_PM1_CULLING_SHIFT) & PVR_TA_PM1_CULLING_MASK;
    mode1 |= (src->depth.write << PVR_TA_PM1_DEPTHWRITE_SHIFT) & PVR_TA_PM1_DEPTHWRITE_MASK;
    mode1 |= (src->txr.enable << PVR_TA_PM1_TXRENABLE_SHIFT) & PVR_TA_PM1_TXRENABLE_MASK;

    dst->mode1 = mode1;

    /* Polygon mode 2 (outside volume) */
    mode2[0]  = (src->blend.src << PVR_TA_PM2_SRCBLEND_SHIFT) & PVR_TA_PM2_SRCBLEND_MASK;
    mode2[0] |= (src->blend.dst << PVR_TA_PM2_DSTBLEND_SHIFT) & PVR_TA_PM2_DSTBLEND_MASK;
    mode2[0] |= (src->blend.src_enable << PVR_TA_PM2_SRCENABLE_SHIFT) & PVR_TA_PM2_SRCENABLE_MASK;
    mode2[0] |= (src->blend.dst_enable << PVR_TA_PM2_DSTENABLE_SHIFT) & PVR_TA_PM2_DSTENABLE_MASK;
    mode2[0] |= (src->gen.fog_type << PVR_TA_PM2_FOG_SHIFT) & PVR_TA_PM2_FOG_MASK;
    mode2[0] |= (src->gen.color_clamp << PVR_TA_PM2_CLAMP_SHIFT) & PVR_TA_PM2_CLAMP_MASK;
    mode2[0] |= (src->gen.alpha << PVR_TA_PM2_ALPHA_SHIFT) & PVR_TA_PM2_ALPHA_MASK;

    if(src->txr.enable == PVR_TEXTURE_DISABLE) {
        dst->mode2_0 = mode2[0];
        dst->mode3_0 = 0;
    }
    else {
        mode2[0] |= (src->txr.alpha << PVR_TA_PM2_TXRALPHA_SHIFT) & PVR_TA_PM2_TXRALPHA_MASK;
        mode2[0] |= (src->txr.uv_flip << PVR_TA_PM2_UVFLIP_SHIFT) & PVR_TA_PM2_UVFLIP_MASK;
        mode2[0] |= (src->txr.uv_clamp << PVR_TA_PM2_UVCLAMP_SHIFT) & PVR_TA_PM2_UVCLAMP_MASK;
        mode2[0] |= (src->txr.filter << PVR_TA_PM2_FILTER_SHIFT) & PVR_TA_PM2_FILTER_MASK;
        mode2[0] |= (src->txr.mipmap_bias << PVR_TA_PM2_MIPBIAS_SHIFT) & PVR_TA_PM2_MIPBIAS_MASK;
        mode2[0] |= (src->txr.env << PVR_TA_PM2_TXRENV_SHIFT) & PVR_TA_PM2_TXRENV_MASK;

        switch(src->txr.width) {
            case 8:
                u = 0;
                break;
            case 16:
                u = 1;
                break;
            case 32:
                u = 2;
                break;
            case 64:
                u = 3;
                break;
            case 128:
                u = 4;
                break;
            case 256:
                u = 5;
                break;
            case 512:
                u = 6;
                break;
            case 1024:
                u = 7;
                break;
            default:
                assert_msg(0, "Invalid texture U size");
                u = 0;
                break;
        }

        switch(src->txr.height) {
            case 8:
                v = 0;
                break;
            case 16:
                v = 1;
                break;
            case 32:
                v = 2;
                break;
            case 64:
                v = 3;
                break;
            case 128:
                v = 4;
                break;
            case 256:
                v = 5;
                break;
            case 512:
                v = 6;
                break;
            case 1024:
                v = 7;
                break;
            default:
                assert_msg(0, "Invalid texture V size");
                v = 0;
                break;
        }

        mode2[0] |= (u << PVR_TA_PM2_USIZE_SHIFT) & PVR_TA_PM2_USIZE_MASK;
        mode2[0] |= (v << PVR_TA_PM2_VSIZE_SHIFT) & PVR_TA_PM2_VSIZE_MASK;

        dst->mode2_0 = mode2[0];

        /* Polygon mode 3 (outside volume) */
        mode3[0]  = (src->txr.mipmap << PVR_TA_PM3_MIPMAP_SHIFT) & PVR_TA_PM3_MIPMAP_MASK;
        mode3[0] |= (src->txr.format << PVR_TA_PM3_TXRFMT_SHIFT) & PVR_TA_PM3_TXRFMT_MASK;

        /* Convert the texture address */
        txr_base = (uint32)src->txr.base;
        txr_base = (txr_base & 0x00fffff8) >> 3;
        mode3[0] |= txr_base;

        dst->mode3_0 = mode3[0];
    }

    /* Polygon mode 2 (within volume) */
    mode2[1]  = (src->blend.src2 << PVR_TA_PM2_SRCBLEND_SHIFT) & PVR_TA_PM2_SRCBLEND_MASK;
    mode2[1] |= (src->blend.dst2 << PVR_TA_PM2_DSTBLEND_SHIFT) & PVR_TA_PM2_DSTBLEND_MASK;
    mode2[1] |= (src->blend.src_enable2 << PVR_TA_PM2_SRCENABLE_SHIFT) & PVR_TA_PM2_SRCENABLE_MASK;
    mode2[1] |= (src->blend.dst_enable2 << PVR_TA_PM2_DSTENABLE_SHIFT) & PVR_TA_PM2_DSTENABLE_MASK;
    mode2[1] |= (src->gen.fog_type2 << PVR_TA_PM2_FOG_SHIFT) & PVR_TA_PM2_FOG_MASK;
    mode2[1] |= (src->gen.color_clamp2 << PVR_TA_PM2_CLAMP_SHIFT) & PVR_TA_PM2_CLAMP_MASK;
    mode2[1] |= (src->gen.alpha2 << PVR_TA_PM2_ALPHA_SHIFT) & PVR_TA_PM2_ALPHA_MASK;

    if(src->txr2.enable == PVR_TEXTURE_DISABLE) {
        dst->mode2_1 = mode2[1];
        dst->mode3_1 = 0;
    }
    else {
        mode2[1] |= (src->txr2.alpha << PVR_TA_PM2_TXRALPHA_SHIFT) & PVR_TA_PM2_TXRALPHA_MASK;
        mode2[1] |= (src->txr2.uv_flip << PVR_TA_PM2_UVFLIP_SHIFT) & PVR_TA_PM2_UVFLIP_MASK;
        mode2[1] |= (src->txr2.uv_clamp << PVR_TA_PM2_UVCLAMP_SHIFT) & PVR_TA_PM2_UVCLAMP_MASK;
        mode2[1] |= (src->txr2.filter << PVR_TA_PM2_FILTER_SHIFT) & PVR_TA_PM2_FILTER_MASK;
        mode2[1] |= (src->txr2.mipmap_bias << PVR_TA_PM2_MIPBIAS_SHIFT) & PVR_TA_PM2_MIPBIAS_MASK;
        mode2[1] |= (src->txr2.env << PVR_TA_PM2_TXRENV_SHIFT) & PVR_TA_PM2_TXRENV_MASK;

        switch(src->txr2.width) {
            case 8:
                u = 0;
                break;
            case 16:
                u = 1;
                break;
            case 32:
                u = 2;
                break;
            case 64:
                u = 3;
                break;
            case 128:
                u = 4;
                break;
            case 256:
                u = 5;
                break;
            case 512:
                u = 6;
                break;
            case 1024:
                u = 7;
                break;
            default:
                assert_msg(0, "Invalid texture U size");
                u = 0;
                break;
        }

        switch(src->txr2.height) {
            case 8:
                v = 0;
                break;
            case 16:
                v = 1;
                break;
            case 32:
                v = 2;
                break;
            case 64:
                v = 3;
                break;
            case 128:
                v = 4;
                break;
            case 256:
                v = 5;
                break;
            case 512:
                v = 6;
                break;
            case 1024:
                v = 7;
                break;
            default:
                assert_msg(0, "Invalid texture V size");
                v = 0;
                break;
        }

        mode2[1] |= (u << PVR_TA_PM2_USIZE_SHIFT) & PVR_TA_PM2_USIZE_MASK;
        mode2[1] |= (v << PVR_TA_PM2_VSIZE_SHIFT) & PVR_TA_PM2_VSIZE_MASK;

        dst->mode2_1 = mode2[1];

        /* Polygon mode 3 (within volume) */
        mode3[1]  = (src->txr2.mipmap << PVR_TA_PM3_MIPMAP_SHIFT) & PVR_TA_PM3_MIPMAP_MASK;
        mode3[1] |= (src->txr2.format << PVR_TA_PM3_TXRFMT_SHIFT) & PVR_TA_PM3_TXRFMT_MASK;

        /* Convert the texture address */
        txr_base = (uint32_t)src->txr2.base;
        txr_base = (txr_base & 0x00fffff8) >> 3;
        mode3[1] |= txr_base;

        dst->mode3_1 = mode3[1];
    }

    dst->d1 = 0xffffffff;
    dst->d2 = 0xffffffff;
}

/* Create a colored polygon context for polygons affected by modifier volumes */
void pvr_poly_cxt_col_mod(pvr_poly_cxt_t *dst, pvr_list_t list) {
    int alpha;

    /* Start off blank */
    memset(dst, 0, sizeof(pvr_poly_cxt_t));

    /* Fill in a few values */
    dst->list_type = list;
    alpha = list > PVR_LIST_OP_MOD;
    dst->fmt.color = PVR_CLRFMT_ARGBPACKED;
    dst->fmt.uv = PVR_UVFMT_32BIT;
    dst->gen.shading = PVR_SHADE_GOURAUD;
    dst->depth.comparison = PVR_DEPTHCMP_GREATER;
    dst->depth.write = PVR_DEPTHWRITE_ENABLE;
    dst->gen.culling = PVR_CULLING_CCW;
    dst->fmt.modifier = PVR_MODIFIER_ENABLE;
    dst->gen.modifier_mode = PVR_MODIFIER_NORMAL;
    dst->txr.enable = PVR_TEXTURE_DISABLE;
    dst->txr2.enable = PVR_TEXTURE_DISABLE;

    if(!alpha) {
        dst->gen.alpha = PVR_ALPHA_DISABLE;
        dst->blend.src = PVR_BLEND_ONE;
        dst->blend.dst = PVR_BLEND_ZERO;
        dst->gen.alpha2 = PVR_ALPHA_DISABLE;
        dst->blend.src2 = PVR_BLEND_ONE;
        dst->blend.dst2 = PVR_BLEND_ZERO;
    }
    else {
        dst->gen.alpha = PVR_ALPHA_ENABLE;
        dst->blend.src = PVR_BLEND_SRCALPHA;
        dst->blend.dst = PVR_BLEND_INVSRCALPHA;
        dst->gen.alpha2 = PVR_ALPHA_ENABLE;
        dst->blend.src2 = PVR_BLEND_SRCALPHA;
        dst->blend.dst2 = PVR_BLEND_INVSRCALPHA;
    }

    dst->blend.src_enable = PVR_BLEND_DISABLE;
    dst->blend.dst_enable = PVR_BLEND_DISABLE;
    dst->gen.fog_type = PVR_FOG_DISABLE;
    dst->gen.color_clamp = PVR_CLRCLAMP_DISABLE;
    dst->blend.src_enable2 = PVR_BLEND_DISABLE;
    dst->blend.dst_enable2 = PVR_BLEND_DISABLE;
    dst->gen.fog_type2 = PVR_FOG_DISABLE;
    dst->gen.color_clamp2 = PVR_CLRCLAMP_DISABLE;
}

/* Create a textured polygon context for polygons affected by modifier
   volumes */
void pvr_poly_cxt_txr_mod(pvr_poly_cxt_t *dst, pvr_list_t list,
                          int textureformat, int tw, int th,
                          pvr_ptr_t textureaddr, int filtering,
                          int textureformat2, int tw2, int th2,
                          pvr_ptr_t textureaddr2, int filtering2) {
    int alpha;

    /* Start off blank */
    memset(dst, 0, sizeof(pvr_poly_cxt_t));

    /* Fill in a few values */
    dst->list_type = list;
    alpha = list > PVR_LIST_OP_MOD;
    dst->fmt.color = PVR_CLRFMT_ARGBPACKED;
    dst->fmt.uv = PVR_UVFMT_32BIT;
    dst->gen.shading = PVR_SHADE_GOURAUD;
    dst->depth.comparison = PVR_DEPTHCMP_GREATER;
    dst->depth.write = PVR_DEPTHWRITE_ENABLE;
    dst->gen.culling = PVR_CULLING_CCW;
    dst->fmt.modifier = PVR_MODIFIER_ENABLE;
    dst->gen.modifier_mode = PVR_MODIFIER_NORMAL;
    dst->txr.enable = PVR_TEXTURE_ENABLE;
    dst->txr2.enable = PVR_TEXTURE_ENABLE;

    if(!alpha) {
        dst->gen.alpha = PVR_ALPHA_DISABLE;
        dst->txr.alpha = PVR_TXRALPHA_ENABLE;
        dst->blend.src = PVR_BLEND_ONE;
        dst->blend.dst = PVR_BLEND_ZERO;
        dst->txr.env = PVR_TXRENV_MODULATE;
        dst->gen.alpha2 = PVR_ALPHA_DISABLE;
        dst->txr2.alpha = PVR_TXRALPHA_ENABLE;
        dst->blend.src2 = PVR_BLEND_ONE;
        dst->blend.dst2 = PVR_BLEND_ZERO;
        dst->txr2.env = PVR_TXRENV_MODULATE;
    }
    else {
        dst->gen.alpha = PVR_ALPHA_ENABLE;
        dst->txr.alpha = PVR_TXRALPHA_ENABLE;
        dst->blend.src = PVR_BLEND_SRCALPHA;
        dst->blend.dst = PVR_BLEND_INVSRCALPHA;
        dst->txr.env = PVR_TXRENV_MODULATEALPHA;
        dst->gen.alpha2 = PVR_ALPHA_ENABLE;
        dst->txr2.alpha = PVR_TXRALPHA_ENABLE;
        dst->blend.src2 = PVR_BLEND_SRCALPHA;
        dst->blend.dst2 = PVR_BLEND_INVSRCALPHA;
        dst->txr2.env = PVR_TXRENV_MODULATEALPHA;
    }

    dst->blend.src_enable = PVR_BLEND_DISABLE;
    dst->blend.dst_enable = PVR_BLEND_DISABLE;
    dst->gen.fog_type = PVR_FOG_DISABLE;
    dst->gen.color_clamp = PVR_CLRCLAMP_DISABLE;
    dst->txr.uv_flip = PVR_UVFLIP_NONE;
    dst->txr.uv_clamp = PVR_UVCLAMP_NONE;
    dst->txr.filter = filtering;
    dst->txr.mipmap_bias = PVR_MIPBIAS_NORMAL;
    dst->txr.width = tw;
    dst->txr.height = th;
    dst->txr.base = textureaddr;
    dst->txr.format = textureformat;
    dst->blend.src_enable2 = PVR_BLEND_DISABLE;
    dst->blend.dst_enable2 = PVR_BLEND_DISABLE;
    dst->gen.fog_type2 = PVR_FOG_DISABLE;
    dst->gen.color_clamp2 = PVR_CLRCLAMP_DISABLE;
    dst->txr2.uv_flip = PVR_UVFLIP_NONE;
    dst->txr2.uv_clamp = PVR_UVCLAMP_NONE;
    dst->txr2.filter = filtering2;
    dst->txr2.mipmap_bias = PVR_MIPBIAS_NORMAL;
    dst->txr2.width = tw2;
    dst->txr2.height = th2;
    dst->txr2.base = textureaddr2;
    dst->txr2.format = textureformat2;
}
