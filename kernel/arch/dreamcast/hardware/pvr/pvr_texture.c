/* KallistiOS ##version##

   pvr_texture.c
   Copyright (C) 2002, 2004 Megan Potter
   Copyright (C) 2024 Andress Barajas

 */

#include <assert.h>
#include <dc/pvr.h>
#include <dc/sq.h>
#include <kos/dbglog.h>
#include <kos/regfield.h>
#include <string.h>
#include "pvr_internal.h"

/*

   Texture handling

   Helper functions for handling texture tasks of various kinds.

*/

void pvr_txr_set_stride(size_t texture_width) {
    uint32_t temp = texture_width / 32;

    /* The width must be a non-zero multiple of 32 that fits in the 5 bits
    of the register field. */
    assert(temp && __is_aligned(texture_width, 32) && !(temp & ~PVR_TXR_STRIDE_MULT));

    /* Pull the reg, mask out current stride, and rewrite with new */
    temp = FIELD_PREP(PVR_TXR_STRIDE_MULT, temp);
    temp |= PVR_GET(PVR_TEXTURE_MODULO) & ~PVR_TXR_STRIDE_MULT;
    PVR_SET(PVR_TEXTURE_MODULO, temp);

    return;
}

size_t pvr_txr_get_stride(void) {
    uint32_t reg = PVR_GET(PVR_TEXTURE_MODULO);
    return FIELD_GET(reg, PVR_TXR_STRIDE_MULT) * 32;
}

/* Load raw texture data from an SH-4 buffer into PVR RAM */
void pvr_txr_load(const void *src, pvr_ptr_t dst, size_t count) {
    count = __align_up(count, 4);
    pvr_sq_load((uint32_t *)dst, (const uint32_t *)src, count, PVR_DMA_VRAM64);
}

/* Linear/iterative twiddling algorithm from Marcus' tatest */
#define TWIDTAB(x) ( (x&1)|((x&2)<<1)|((x&4)<<2)|((x&8)<<3)|((x&16)<<4)| \
                     ((x&32)<<5)|((x&64)<<6)|((x&128)<<7)|((x&256)<<8)|((x&512)<<9) )
#define TWIDOUT(x, y) ( TWIDTAB((y)) | (TWIDTAB((x)) << 1) )

#define MIN(a, b) ( (a)<(b)? (a):(b) )

/*
   Load texture data from an SH-4 buffer into PVR RAM, twiddling it
   in the process.

   This is a modified version of Vincent Penne's general twiddling
   function. The texture can be 16bpp, 8bpp, or 4bpp (i.e., paletted).
   The rectangle does not need to be a square (not tested with h>w, but
   this is just a matter of a rotation by PI/2 and UV swapping).

   - w and h must be a power of 2
   - flags must be a logical OR of the various texture loading
     flags available:
       PVR_TXRLOAD_4BPP, _8BPP, _16BPP, _32BPP (not supported yet)
       PVR_TXRLOAD_VQ (not supported yet)
       PVR_TXRLOAD_INVERT

*/
void pvr_txr_load_ex(const void *src, pvr_ptr_t dst, uint32_t w, uint32_t h,
                     uint32_t flags) {
    uint32_t x, y, yout, min, mask, bpp, invert;

    /* Make sure we're attempting something we can do */
    switch(flags & PVR_TXRLOAD_FMT_MASK) {
        case PVR_TXRLOAD_4BPP:
            bpp = 4;
            break;
        case PVR_TXRLOAD_8BPP:
            bpp = 8;
            break;
        case PVR_TXRLOAD_16BPP:
            bpp = 16;
            break;
        default:
            assert_msg(0, "Invalid format specifier in `flags'");
            bpp = 8;
    }

    assert_msg(!(flags & PVR_TXRLOAD_VQ_LOAD), "VQ compression on the fly not supported yet");
    invert = (flags & PVR_TXRLOAD_INVERT_Y) ? 1 : 0;

    min = MIN(w, h);
    mask = min - 1;

    switch(bpp) {
        case 4: {
            uint8_t * pixels;
            uint16_t * vtex;
            pixels = (uint8_t *) src;
            vtex = (uint16_t *)dst;

            for(y = 0; y < h; y += 2) {
                if(!invert)
                    yout = y;
                else
                    yout = ((h - 1) - y);

                for(x = 0; x < w; x += 2) {
                    vtex[TWIDOUT((x & mask) / 2, (yout & mask) / 2) +
                         (x / min + yout / min)*min * min / 4] =
                             (pixels[(x + y * w) >> 1] & 15) | ((pixels[(x + (y + 1) * w) >> 1] & 15) << 4) |
                             ((pixels[(x + y * w) >> 1] >> 4) << 8) | ((pixels[(x + (y + 1) * w) >> 1] >> 4) << 12);
                }
            }
        }
        break;
        case 8: {
            uint8_t * pixels;
            uint16_t * vtex;
            pixels = (uint8_t *) src;
            vtex = (uint16_t *)dst;

            for(y = 0; y < h; y += 2) {
                if(!invert)
                    yout = y;
                else
                    yout = ((h - 1) - y);

                for(x = 0; x < w; x++) {
                    vtex[TWIDOUT((yout & mask) / 2, x & mask) +
                         (x / min + yout / min)*min * min / 2] =
                             pixels[y * w + x] | (pixels[(y + 1) * w + x] << 8);
                }
            }
        }
        break;
        case 16: {
            uint16_t * pixels;
            uint16_t * vtex;
            pixels = (uint16_t *) src;
            vtex = (uint16_t *)dst;

            for(y = 0; y < h; y++) {
                if(!invert)
                    yout = y;
                else
                    yout = ((h - 1) - y);

                for(x = 0; x < w; x++) {
                    vtex[TWIDOUT(x & mask, yout & mask) +
                         (x / min + yout / min)*min * min] = pixels[y * w + x];
                }
            }
        }
        break;
    }
}

/* Load a KOS Platform Independent Image (subject to restraint checking) */
void pvr_txr_load_kimg(const kos_img_t *img, pvr_ptr_t dst, uint32_t flags) {
    uint32_t fmt, w, h;

    /* First check and make sure it's a format we can use */
    fmt = KOS_IMG_FMT_I(img->fmt) & KOS_IMG_FMT_MASK;
    assert_msg(fmt == KOS_IMG_FMT_RGB565 || fmt == KOS_IMG_FMT_ARGB4444
               || fmt == KOS_IMG_FMT_ARGB1555
               || fmt == KOS_IMG_FMT_PAL4BPP
               || fmt == KOS_IMG_FMT_PAL8BPP, "Unsupported format in input kos_img_t");

    /* Second, check to make sure it's a proper power of 2 we can use */
    w = img->w;
    h = img->h;
    assert_msg(w == 8 || w == 16 || w == 32 || w == 64 || w == 128
               || w == 256 || w == 512 || w == 1024, "Non power-of-2 image width in input kos_img_t");
    assert_msg(h == 8 || h == 16 || h == 32 || h == 64 || h == 128
               || h == 256 || h == 512 || h == 1024, "Non power-of-2 image height in input kos_img_t");

    /* Convert it to a PVR image type */
    switch(fmt) {
        case KOS_IMG_FMT_RGB565:
        case KOS_IMG_FMT_ARGB4444:
        case KOS_IMG_FMT_ARGB1555:
            fmt = PVR_TXRLOAD_16BPP;
            break;
        case KOS_IMG_FMT_PAL4BPP:
            fmt = PVR_TXRLOAD_4BPP;
            break;
        case KOS_IMG_FMT_PAL8BPP:
            fmt = PVR_TXRLOAD_8BPP;
            break;
    }

    /* Make sure the format part of the flags is clean */
    flags = (flags & ~PVR_TXRLOAD_FMT_MASK) | fmt;

    /* Call down */
    if((flags & PVR_TXRLOAD_FMT_VQ) || (flags & PVR_TXRLOAD_FMT_TWIDDLED) ||
            (KOS_IMG_FMT_D(img->fmt) & PVR_TXRLOAD_FMT_VQ) ||
            (KOS_IMG_FMT_D(img->fmt) & PVR_TXRLOAD_FMT_TWIDDLED)) {
        if(flags & PVR_TXRLOAD_INVERT_Y)
            assert_msg(0, "Inverted, non-twiddled loading not supported yet");
        else {
            /* We only enable DMA here for now since it sort of changes things
               to have to allocate an intermediary buffer. */
            if(flags & PVR_TXRLOAD_DMA) {
                mutex_lock((mutex_t *)&pvr_state.dma_lock);
                pvr_txr_load_dma(img->data, dst, img->byte_count,
                                 (flags & PVR_TXRLOAD_NONBLOCK) ? 1 : 0, NULL, 0);
                mutex_unlock((mutex_t *)&pvr_state.dma_lock);
            }
            else if(flags & PVR_TXRLOAD_SQ) {
                pvr_txr_load(img->data, dst, img->byte_count);
            }
            else {
                memcpy(dst, img->data, img->byte_count);
            }
        }
    }
    else {
        pvr_txr_load_ex(img->data, dst, w, h, flags);
    }
}
