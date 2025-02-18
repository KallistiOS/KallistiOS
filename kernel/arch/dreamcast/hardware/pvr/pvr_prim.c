/* KallistiOS ##version##

   pvr_prim.c
   Copyright (C) 2002 Megan Potter

 */

#include <assert.h>
#include <string.h>
#include <arch/cache.h>
#include <dc/pvr.h>
#include "pvr_internal.h"

/*

   Primitive handling

   These functions help you prepare primitives for loading into the
   PVR for scene processing.

*/

/* Compile a polygon context into a polygon header */
void pvr_poly_compile(pvr_poly_hdr_t *dst, const pvr_poly_cxt_t *src) {
    uint32_t txr_base, cmd;
    /* Temporary variables we can read-write-modify, since we cannot do so from
       within the SQs, and we want to be able to compile this header from a PVR
       DR API submission target. */
    uint32_t mode2, mode3;

    /* Basically we just take each parameter, clip it, shift it
       into place, and OR it into the final result. */

    /* The base values for CMD */
    cmd = PVR_CMD_POLYHDR
        | FIELD_PREP(PVR_TA_CMD_TXRENABLE, src->txr.enable)
        | FIELD_PREP(PVR_TA_CMD_TYPE, src->list_type)
        | FIELD_PREP(PVR_TA_CMD_CLRFMT, src->fmt.color)
        | FIELD_PREP(PVR_TA_CMD_SHADE, src->gen.shading)
        | FIELD_PREP(PVR_TA_CMD_UVFMT, src->fmt.uv)
        | FIELD_PREP(PVR_TA_CMD_USERCLIP, src->gen.clip_mode)
        | FIELD_PREP(PVR_TA_CMD_MODIFIER, src->fmt.modifier)
        | FIELD_PREP(PVR_TA_CMD_MODIFIERMODE, src->gen.modifier_mode)
        | FIELD_PREP(PVR_TA_CMD_SPECULAR, src->gen.specular);

    /* pvr_poly_hdr_t is cacheline-aligned and we're writing all 32 bytes:
     * we can allocate a dirty cache line */
    dcache_alloc_block(dst, cmd);

    /* Polygon mode 1 */
    dst->mode1 = FIELD_PREP(PVR_TA_PM1_DEPTHCMP, src->depth.comparison)
        | FIELD_PREP(PVR_TA_PM1_CULLING, src->gen.culling)
        | FIELD_PREP(PVR_TA_PM1_DEPTHWRITE, src->depth.write)
        | FIELD_PREP(PVR_TA_PM1_TXRENABLE, src->txr.enable);

    /* Polygon mode 2 */
    mode2 = FIELD_PREP(PVR_TA_PM2_SRCBLEND, src->blend.src)
        | FIELD_PREP(PVR_TA_PM2_DSTBLEND, src->blend.dst)
        | FIELD_PREP(PVR_TA_PM2_SRCENABLE, src->blend.src_enable)
        | FIELD_PREP(PVR_TA_PM2_DSTENABLE, src->blend.dst_enable)
        | FIELD_PREP(PVR_TA_PM2_FOG, src->gen.fog_type)
        | FIELD_PREP(PVR_TA_PM2_CLAMP, src->gen.color_clamp)
        | FIELD_PREP(PVR_TA_PM2_ALPHA, src->gen.alpha);

    if(src->txr.enable == PVR_TEXTURE_DISABLE) {
        mode3 = 0;
    }
    else {
        assert_msg(__builtin_popcount(src->txr.width) == 1
		   && src->txr.width <= 1024, "Invalid texture U size");
        assert_msg(__builtin_popcount(src->txr.height) == 1
		   && src->txr.height <= 1024, "Invalid texture V size");

        mode2 |= FIELD_PREP(PVR_TA_PM2_TXRALPHA, src->txr.alpha)
            | FIELD_PREP(PVR_TA_PM2_UVFLIP, src->txr.uv_flip)
            | FIELD_PREP(PVR_TA_PM2_UVCLAMP, src->txr.uv_clamp)
            | FIELD_PREP(PVR_TA_PM2_FILTER, src->txr.filter)
            | FIELD_PREP(PVR_TA_PM2_MIPBIAS, src->txr.mipmap_bias)
            | FIELD_PREP(PVR_TA_PM2_TXRENV, src->txr.env)
            | FIELD_PREP(PVR_TA_PM2_USIZE, __builtin_ctz(src->txr.width) - 3)
            | FIELD_PREP(PVR_TA_PM2_VSIZE, __builtin_ctz(src->txr.height) - 3);

        /* Convert the texture address */
        txr_base = (uint32_t)src->txr.base;
        txr_base = (txr_base & 0x00fffff8) >> 3;

        /* Polygon mode 3 */
        mode3 = FIELD_PREP(PVR_TA_PM3_MIPMAP, src->txr.mipmap)
            | src->txr.format
            | txr_base;
    }

    dst->mode2 = mode2;
    dst->mode3 = mode3;

    if(src->fmt.modifier && src->gen.modifier_mode) {
        /* If we're affected by a modifier volume, silently promote the header
           to the one that is affected by a modifier volume. */
        dst->d1 = mode2;
        dst->d2 = mode3;
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
    uint32_t txr_base, cmd, mode2, mode3;

    /* Basically we just take each parameter, clip it, shift it
       into place, and OR it into the final result. */

    /* The base values for CMD */
    cmd = PVR_CMD_SPRITE
        | FIELD_PREP(PVR_TA_CMD_TXRENABLE, src->txr.enable)
        | FIELD_PREP(PVR_TA_CMD_TYPE, src->list_type)
        | FIELD_PREP(PVR_TA_CMD_UVFMT, PVR_UVFMT_16BIT)
        | FIELD_PREP(PVR_TA_CMD_USERCLIP, src->gen.clip_mode)
        | FIELD_PREP(PVR_TA_CMD_SPECULAR, src->gen.specular);

    /* pvr_sprite_hdr_t is cacheline-aligned and we're writing all 32 bytes:
     * we can allocate a dirty cache line */
    dcache_alloc_block(dst, cmd);

    /* Polygon mode 1 */
    dst->mode1 = FIELD_PREP(PVR_TA_PM1_DEPTHCMP, src->depth.comparison)
        | FIELD_PREP(PVR_TA_PM1_CULLING, src->gen.culling)
        | FIELD_PREP(PVR_TA_PM1_DEPTHWRITE, src->depth.write)
        | FIELD_PREP(PVR_TA_PM1_TXRENABLE, src->txr.enable);

    /* Polygon mode 2 */
    mode2 = FIELD_PREP(PVR_TA_PM2_SRCBLEND, src->blend.src)
        | FIELD_PREP(PVR_TA_PM2_DSTBLEND, src->blend.dst)
        | FIELD_PREP(PVR_TA_PM2_SRCENABLE, src->blend.src_enable)
        | FIELD_PREP(PVR_TA_PM2_DSTENABLE, src->blend.dst_enable)
        | FIELD_PREP(PVR_TA_PM2_FOG, src->gen.fog_type)
        | FIELD_PREP(PVR_TA_PM2_CLAMP, src->gen.color_clamp)
        | FIELD_PREP(PVR_TA_PM2_ALPHA, src->gen.alpha);

    if(src->txr.enable == PVR_TEXTURE_DISABLE) {
        mode3 = 0;
    }
    else {
        assert_msg(__builtin_popcount(src->txr.width) == 1
		   && src->txr.width <= 1024, "Invalid texture U size");
        assert_msg(__builtin_popcount(src->txr.height) == 1
		   && src->txr.height <= 1024, "Invalid texture V size");

        mode2 |= FIELD_PREP(PVR_TA_PM2_TXRALPHA, src->txr.alpha)
            | FIELD_PREP(PVR_TA_PM2_UVFLIP, src->txr.uv_flip)
            | FIELD_PREP(PVR_TA_PM2_UVCLAMP, src->txr.uv_clamp)
            | FIELD_PREP(PVR_TA_PM2_FILTER, src->txr.filter)
            | FIELD_PREP(PVR_TA_PM2_MIPBIAS, src->txr.mipmap_bias)
            | FIELD_PREP(PVR_TA_PM2_TXRENV, src->txr.env)
            | FIELD_PREP(PVR_TA_PM2_USIZE, __builtin_ctz(src->txr.width) - 3)
            | FIELD_PREP(PVR_TA_PM2_VSIZE, __builtin_ctz(src->txr.height) - 3);

        /* Convert the texture address */
        txr_base = (uint32_t)src->txr.base;
        txr_base = (txr_base & 0x00fffff8) >> 3;

        /* Polygon mode 3 */
        mode3 = FIELD_PREP(PVR_TA_PM3_MIPMAP, src->txr.mipmap)
            | src->txr.format
            | txr_base;
    }

    dst->mode2 = mode2;
    dst->mode3 = mode3;

    dst->argb = 0xFFFFFFFF;
    dst->oargb = 0x00000000;
}

void pvr_mod_compile(pvr_mod_hdr_t *dst, pvr_list_t list, uint32 mode,
                     uint32 cull) {
    uint32_t cmd;

    cmd = PVR_CMD_MODIFIER
        | FIELD_PREP(PVR_TA_CMD_TYPE, list);

    /* pvr_mod_hdr_t is cacheline-aligned and we're writing all 32 bytes:
     * we can allocate a dirty cache line */
    dcache_alloc_block(dst, cmd);

    dst->mode1 = FIELD_PREP(PVR_TA_PM1_MODIFIERINST, mode)
        | FIELD_PREP(PVR_TA_PM1_CULLING, cull);

    dst->d1 = dst->d2 = dst->d3 = dst->d4 = dst->d5 = dst->d6 = 0;
}

/* Compile a polygon context into a polygon header that is affected by
   modifier volumes */
void pvr_poly_mod_compile(pvr_poly_mod_hdr_t *dst, const pvr_poly_cxt_t *src) {
    uint32_t txr_base, cmd;
    uint32_t mode2, mode3;

    /* Basically we just take each parameter, clip it, shift it
       into place, and OR it into the final result. */

    /* The base values for CMD */
    cmd = PVR_CMD_POLYHDR
        | FIELD_PREP(PVR_TA_CMD_TXRENABLE, src->txr.enable)
        | FIELD_PREP(PVR_TA_CMD_TYPE, src->list_type)
        | FIELD_PREP(PVR_TA_CMD_CLRFMT, src->fmt.color)
        | FIELD_PREP(PVR_TA_CMD_SHADE, src->gen.shading)
        | FIELD_PREP(PVR_TA_CMD_UVFMT, src->fmt.uv)
        | FIELD_PREP(PVR_TA_CMD_USERCLIP, src->gen.clip_mode)
        | FIELD_PREP(PVR_TA_CMD_MODIFIER, src->fmt.modifier)
        | FIELD_PREP(PVR_TA_CMD_MODIFIERMODE, src->gen.modifier_mode)
        | FIELD_PREP(PVR_TA_CMD_SPECULAR, src->gen.specular);

    /* pvr_poly_mod_hdr_t is cacheline-aligned and we're writing all 32 bytes:
     * we can allocate a dirty cache line */
    dcache_alloc_block(dst, cmd);

    /* Polygon mode 1 */
    dst->mode1 = FIELD_PREP(PVR_TA_PM1_DEPTHCMP, src->depth.comparison)
        | FIELD_PREP(PVR_TA_PM1_CULLING, src->gen.culling)
        | FIELD_PREP(PVR_TA_PM1_DEPTHWRITE, src->depth.write)
        | FIELD_PREP(PVR_TA_PM1_TXRENABLE, src->txr.enable);

    /* Polygon mode 2 (outside volume) */
    mode2 = FIELD_PREP(PVR_TA_PM2_SRCBLEND, src->blend.src)
        | FIELD_PREP(PVR_TA_PM2_DSTBLEND, src->blend.dst)
        | FIELD_PREP(PVR_TA_PM2_SRCENABLE, src->blend.src_enable)
        | FIELD_PREP(PVR_TA_PM2_DSTENABLE, src->blend.dst_enable)
        | FIELD_PREP(PVR_TA_PM2_FOG, src->gen.fog_type)
        | FIELD_PREP(PVR_TA_PM2_CLAMP, src->gen.color_clamp)
        | FIELD_PREP(PVR_TA_PM2_ALPHA, src->gen.alpha);

    if(src->txr.enable == PVR_TEXTURE_DISABLE) {
        mode3 = 0;
    }
    else {
        assert_msg(__builtin_popcount(src->txr.width) == 1
		   && src->txr.width <= 1024, "Invalid texture U size");
        assert_msg(__builtin_popcount(src->txr.height) == 1
		   && src->txr.height <= 1024, "Invalid texture V size");

        mode2 |= FIELD_PREP(PVR_TA_PM2_TXRALPHA, src->txr.alpha)
            | FIELD_PREP(PVR_TA_PM2_UVFLIP, src->txr.uv_flip)
            | FIELD_PREP(PVR_TA_PM2_UVCLAMP, src->txr.uv_clamp)
            | FIELD_PREP(PVR_TA_PM2_FILTER, src->txr.filter)
            | FIELD_PREP(PVR_TA_PM2_MIPBIAS, src->txr.mipmap_bias)
            | FIELD_PREP(PVR_TA_PM2_TXRENV, src->txr.env)
            | FIELD_PREP(PVR_TA_PM2_USIZE, __builtin_ctz(src->txr.width) - 3)
            | FIELD_PREP(PVR_TA_PM2_VSIZE, __builtin_ctz(src->txr.height) - 3);

        /* Convert the texture address */
        txr_base = (uint32_t)src->txr.base;
        txr_base = (txr_base & 0x00fffff8) >> 3;

        /* Polygon mode 3 */
        mode3 = FIELD_PREP(PVR_TA_PM3_MIPMAP, src->txr.mipmap)
            | src->txr.format
            | txr_base;
    }

    dst->mode2_0 = mode2;
    dst->mode3_0 = mode3;

    /* Polygon mode 2 (within volume) */
    mode2 = FIELD_PREP(PVR_TA_PM2_SRCBLEND, src->blend.src2)
        | FIELD_PREP(PVR_TA_PM2_DSTBLEND, src->blend.dst2)
        | FIELD_PREP(PVR_TA_PM2_SRCENABLE, src->blend.src_enable2)
        | FIELD_PREP(PVR_TA_PM2_DSTENABLE, src->blend.dst_enable2)
        | FIELD_PREP(PVR_TA_PM2_FOG, src->gen.fog_type2)
        | FIELD_PREP(PVR_TA_PM2_CLAMP, src->gen.color_clamp2)
        | FIELD_PREP(PVR_TA_PM2_ALPHA, src->gen.alpha2);

    if(src->txr2.enable == PVR_TEXTURE_DISABLE) {
        mode3 = 0;
    }
    else {
        assert_msg(__builtin_popcount(src->txr2.width) == 1
		   && src->txr2.width <= 1024, "Invalid texture U size");
        assert_msg(__builtin_popcount(src->txr2.height) == 1
		   && src->txr2.height <= 1024, "Invalid texture V size");

        mode2 |= FIELD_PREP(PVR_TA_PM2_TXRALPHA, src->txr2.alpha)
            | FIELD_PREP(PVR_TA_PM2_UVFLIP, src->txr2.uv_flip)
            | FIELD_PREP(PVR_TA_PM2_UVCLAMP, src->txr2.uv_clamp)
            | FIELD_PREP(PVR_TA_PM2_FILTER, src->txr2.filter)
            | FIELD_PREP(PVR_TA_PM2_MIPBIAS, src->txr2.mipmap_bias)
            | FIELD_PREP(PVR_TA_PM2_TXRENV, src->txr2.env)
            | FIELD_PREP(PVR_TA_PM2_USIZE, __builtin_ctz(src->txr2.width) - 3)
            | FIELD_PREP(PVR_TA_PM2_VSIZE, __builtin_ctz(src->txr2.height) - 3);

        /* Convert the texture address */
        txr_base = (uint32_t)src->txr2.base;
        txr_base = (txr_base & 0x00fffff8) >> 3;

        /* Polygon mode 3 */
        mode3 = FIELD_PREP(PVR_TA_PM3_MIPMAP, src->txr2.mipmap)
            | src->txr2.format
            | txr_base;
    }

    dst->mode2_1 = mode2;
    dst->mode3_1 = mode3;

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
