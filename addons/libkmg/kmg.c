/* KallistiOS ##version##

   KOS image format loader library

   kmg.c
   Copyright (C)2003 Megan Potter

*/

#include <kos.h>
#include <assert.h>
#include <kmg/kmg.h>

/* This (tiny) library implements a "KMG" format loader. This format is
   intended to be a KOS-specific format for loading platform specific
   image formats directly from the VFS. To put this more clearly, this
   handles things like pre-twiddled and pre-VQ'd textures on the Dreamcast,
   and any similar formats on other platforms. Currently only Dreamcast
   formats are supported.

   Note also that the allocated buffer for the image will be 32-byte
   aligned so you can DMA the textures straight to VRAM. */

int kmg_to_img(const char *fn, kos_img_t *rv) {
    file_t          f;
    kmg_header_t    hdr;
    int             dep;
    void           *map;

    assert( rv != NULL );

    /* Open the file */
    f = fs_open(fn, O_RDONLY);
    if (f == FILEHND_INVALID) {
        dbglog(DBG_ERROR, "kmg_to_img: can't open file '%s'\n", fn);
        return -1;
    }

    /* Read the header */
    if (fs_read(f, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        fs_close(f);
        dbglog(DBG_ERROR, "kmg_to_img: can't read header from file '%s'\n", fn);
        return -2;
    }

    /* Verify a few things */
    if (hdr.magic != KMG_MAGIC || hdr.version != KMG_VERSION ||
        hdr.platform != KMG_PLAT_DC)
    {
        fs_close(f);
        dbglog(DBG_ERROR, "kmg_to_img: file '%s' is incompatible:\n"
            "   magic %08lx version %d platform %d\n",
            fn, hdr.magic, (int)hdr.version, (int)hdr.platform);
        return -3;
    }

    /* See if we can mmap the thing. If so, then we'll just make it
       a non-owned buffer that way and save some transfers. */
    map = fs_mmap(f);
    if (map) {
        rv->data = ((uint8_t *)map) + sizeof(hdr);
    }

    /* Setup the kimg struct */
    rv->w = hdr.width;
    rv->h = hdr.height;

    dep = 0;
    if (hdr.format & KMG_DCFMT_VQ)
        dep |= PVR_TXRLOAD_FMT_VQ;
    if (hdr.format & KMG_DCFMT_TWIDDLED)
        dep |= PVR_TXRLOAD_FMT_TWIDDLED;

    switch (hdr.format & KMG_DCFMT_MASK) {
    case KMG_DCFMT_RGB565:
        rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_RGB565, dep);
        break;

    case KMG_DCFMT_ARGB4444:
        rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_ARGB4444, dep);
        break;

    case KMG_DCFMT_ARGB1555:
        rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_ARGB1555, dep);
        break;

    case KMG_DCFMT_YUV422:
        rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_YUV422, dep);
        break;

    case KMG_DCFMT_BUMP:
        /* XXX */
        rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_RGB565, dep);
        break;

    case KMG_DCFMT_4BPP_PAL:
    case KMG_DCFMT_8BPP_PAL:
    default:
        assert_msg( 0, "currently-unsupported KMG pixel format" );
        fs_close(f);
        free(rv->data);
        return -5;
    }

    rv->byte_count = hdr.byte_count;

    /* And load the rest of it if necessary */
    if (map) {
        rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_I(rv->fmt) | KOS_IMG_NOT_OWNER,
            KOS_IMG_FMT_D(rv->fmt));
    } else {
        rv->data = malloc(hdr.byte_count);
        if (!rv->data) {
            dbglog(DBG_ERROR, "kmg_to_img: can't malloc(%d) while loading '%s'\n",
                (int)hdr.byte_count, fn);
            fs_close(f);
            return -4;
        }
        if (fs_read(f, rv->data, rv->byte_count) != (ssize_t)rv->byte_count) {
            dbglog(DBG_ERROR, "kmg_to_img: can't read %d bytes while loading '%s'\n",
                (int)hdr.byte_count, fn);
            fs_close(f);
            free(rv->data);
            return -6;
        }
    }

    /* Ok, all done */
    fs_close(f);

    /* If the byte count is not a multiple of 32, bump it up as well.
       This is for DMA/SQ usage. */
    rv->byte_count = (rv->byte_count + 31) & ~31;

    return 0;
}
