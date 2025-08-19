/* KallistiOS ##version##

   video.c

   Copyright (C) 2001 Anders Clerwall (scav)
   Copyright (C) 2000-2001 Megan Potter
   Copyright (C) 2023-2024 Donald Haase
 */

#include <dc/video.h>
#include <dc/pvr.h>
#include <dc/sq.h>
#include <kos/dbglog.h>
#include <kos/platform.h>
#include <string.h>
#include <stdio.h>

/*-----------------------------------------------------------------------------*/
/* This table is indexed w/ DM_* */
vid_mode_t vid_builtin[DM_MODE_COUNT] = {
    /* NULL mode.. */
    /* DM_INVALID = 0 */
    { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 },

    /* 320x240 VGA 60Hz */
    /* DM_320x240_VGA */
    {
        DM_320x240,
        320, 240,
        VID_PIXELDOUBLE | VID_LINEDOUBLE,
        CT_VGA,
        0,
        262, 857,
        172, 40,
        21, 260,
        141, 843,
        24, 263,
        0, 1, 0
    },

    /* 320x240 NTSC 60Hz */
    /* DM_320x240_NTSC */
    {
        DM_320x240,
        320, 240,
        VID_PIXELDOUBLE | VID_LINEDOUBLE,
        CT_ANY,
        0,
        262, 857,
        164, 24,
        21, 260,
        141, 843,
        24, 263,
        0, 1, 0
    },

    /* 640x480 VGA 60Hz */
    /* DM_640x480_VGA */
    {
        DM_640x480,
        640, 480,
        0,
        CT_VGA,
        0,
        524, 857,
        172, 40,
        21, 260,
        126, 837,
        36, 516,
        0, 1, 0
    },

    /* 640x480 NTSC 60Hz IL */
    /* DM_640x480_NTSC_IL */
    {
        DM_640x480,
        640, 480,
        VID_INTERLACE,
        CT_ANY,
        0,
        524, 857,
        164, 18,
        21, 260,
        126, 837,
        36, 516,
        0, 1, 0
    },

    /* 640x480 PAL 50Hz IL */
    /* DM_640x480_PAL_IL */
    {
        DM_640x480,
        640, 480,
        VID_INTERLACE | VID_PAL,
        CT_ANY,
        0,
        624, 863,
        174, 45,
        21, 260,
        141, 843,
        44, 620,
        0, 1, 0
    },

    /* 256x256 PAL 50Hz IL (seems to output the same w/o VID_PAL, ie. in NTSC IL mode) */
    /* DM_256x256_PAL_IL */
    {
        DM_256x256,
        256, 256,
        VID_PIXELDOUBLE | VID_LINEDOUBLE | VID_INTERLACE | VID_PAL,
        CT_ANY,
        0,
        624, 863,
        226, 37,
        21, 260,
        141, 843,
        44, 620,
        0, 1, 0
    },

    /* 768x480 NTSC 60Hz IL (thanks DCGrendel) */
    /* DM_768x480_NTSC_IL */
    {
        DM_768x480,
        768, 480,
        VID_INTERLACE,
        CT_ANY,
        0,
        524, 857,
        96, 18,
        21, 260,
        46, 837,
        36, 516,
        0, 1, 0
    },

    /* 768x576 PAL 50Hz IL (DCG) */
    /* DM_768x576_PAL_IL */
    {
        DM_768x576,
        768, 576,
        VID_INTERLACE | VID_PAL,
        CT_ANY,
        0,
        624, 863,
        88, 16,
        24, 260,
        54, 843,
        44, 620,
        0, 1, 0
    },

    /* 768x480 PAL 50Hz IL */
    /* DM_768x480_PAL_IL */
    {
        DM_768x480,
        768, 480,
        VID_INTERLACE | VID_PAL,
        CT_ANY,
        0,
        624, 863,
        88, 16,
        24, 260,
        54, 843,
        44, 620,
        0, 1, 0
    },

    /* 320x240 PAL 50Hz (thanks Marco Martins aka Mekanaizer) */
    /* DM_320x240_PAL */
    {
        DM_320x240,
        320, 240,
        VID_PIXELDOUBLE | VID_LINEDOUBLE | VID_PAL,
        CT_ANY,
        0,
        312, 863,
        174, 45,
        21, 260,
        141, 843,
        44, 620,
        0, 1, 0
    },

    /* END */
    /* DM_SENTINEL */
    { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 }

    /* DM_MODE_COUNT */
};

/*-----------------------------------------------------------------------------*/
static vid_mode_t  currmode = { 0 };
vid_mode_t  *vid_mode = 0;
uint16_t      *vram_s;
uint32_t      *vram_l;

/*-----------------------------------------------------------------------------*/
/* Checks the attached cable type (to the A/V port). Returns
   one of the following:
     0 == VGA
     1 == (nothing)
     2 == RGB
     3 == Composite

   This is a direct port of Marcus' assembly function of the
   same name.

   [This is the old KOS function by Megan.]
*/
int8_t vid_check_cable(void) {
    volatile uint32_t * porta = (vuint32 *)0xff80002c;

    if(KOS_PLATFORM_IS_NAOMI) {
        /* XXXX: This still needs to be figured out for NAOMI. For now, assume
           VGA mode. */
        return CT_VGA;
    }

    *porta = (*porta & 0xfff0ffff) | 0x000a0000;

    /* Read port8 and port9 */
    return (*((volatile uint16_t*)(porta + 1)) >> 8) & 3;
}

/*-----------------------------------------------------------------------------*/
void vid_set_mode(int dm, vid_pixel_mode_t pm) {
    vid_mode_t mode;
    int i, ct, found, mb;

    ct = vid_check_cable();

    /* Remove the multi-buffering flag from the mode, if its present, and save
       the state of that flag. */
    mb = dm & DM_MULTIBUFFER;
    dm &= ~DM_MULTIBUFFER;

    /* Check to see if we should use a direct mode index, a generic
       mode check, or if it's just invalid. */
    if(dm > DM_INVALID && dm < DM_SENTINEL) {
        memcpy(&mode, &vid_builtin[dm], sizeof(vid_mode_t));
    }
    else if(dm >= DM_GENERIC_FIRST && dm <= DM_GENERIC_LAST) {
        found = 0;

        for(i = 1; i < DM_SENTINEL; i++) {
            /* Is it the right generic mode? */
            if(vid_builtin[i].generic != dm)
                continue;

            /* Do we have the right cable type? */
            if(vid_builtin[i].cable_type != CT_ANY &&
                    vid_builtin[i].cable_type != ct)
                continue;

            /* Ok, nothing else to check right now -- we've got our mode */
            memcpy(&mode, &vid_builtin[i], sizeof(vid_mode_t));
            found = 1;
            break;
        }

        if(!found) {
            dbglog(DBG_ERROR, "vid_set_mode: invalid generic mode %04x\n", dm);
            return;
        }
    }
    else {
        dbglog(DBG_ERROR, "vid_set_mode: invalid mode specifier %04x\n", dm);
        return;
    }

    /* We set this here so actual mode is bit-depth independent.. */
    mode.pm = pm;

    /* Calculate basic size needed for a framebuffer */
    mode.fb_size = (mode.width * mode.height) * vid_pmode_bpp[mode.pm];

    /* Ensure the FBs are 32-bit aligned */
    if(mode.fb_size % 4)
        mode.fb_size = (mode.fb_size + 4) & ~3;

    if(mb == DM_MULTIBUFFER) {
        /* Fill vram with framebuffers */
        mode.fb_count = PVR_RAM_SIZE / mode.fb_size;
    }

    /* This is also to be generic */
    mode.cable_type = ct;

    /* This will make a private copy of our "mode" */
    vid_set_mode_ex(&mode);
}

enum pvr_pm_modes {
    PVR_PM_XRGB1555,
    PVR_PM_RGB565,
    PVR_PM_ARGB4444,
    PVR_PM_ARGB1555,
    PVR_PM_RGB888,
    PVR_PM_XRGB8888,
    PVR_PM_ARGB8888,
    PVR_PM_DITHER = 8,
};

static const unsigned int vid_bpp_to_pvr_cfg2[] = {
    [PM_RGB555] = PVR_PM_XRGB1555 | PVR_PM_DITHER,
    [PM_RGB565] = PVR_PM_RGB565 | PVR_PM_DITHER,
    [PM_RGB888P] = PVR_PM_RGB888,
    [PM_RGB0888] = PVR_PM_XRGB8888,
};

/*-----------------------------------------------------------------------------*/
void vid_set_mode_ex(vid_mode_t *mode) {
    uint16_t ct;
    uint32_t data;


    /* Verify cable type for video mode. */
    ct = vid_check_cable();

    if(mode->cable_type != CT_ANY) {
        if(mode->cable_type != ct) {
            /* Maybe this should have the ability to be forced (thru param)
               so you can set a mode with VGA params with RGB cable type? */
            /*ct=mode->cable_type; */
            dbglog(DBG_ERROR, "vid_set_mode: Mode not allowed for this cable type (%i!=%i)\n", mode->cable_type, ct);
            return;
        }
    }

    /* Blank screen and reset display enable (looks nicer) */
    vid_set_enabled(false);

    /* Also clear any set border color now */
    vid_border_color(0, 0, 0);

    /* Clear interlace flag if VGA (this maybe should be in here?) */
    if(ct == CT_VGA) {
        mode->flags &= ~VID_INTERLACE;

        if(mode->flags & VID_LINEDOUBLE)
            mode->scanlines *= 2;
    }

    dbglog(DBG_INFO, "vid_set_mode: %ix%i%s %s with %i framebuffers.\n", mode->width, mode->height,
           (mode->flags & VID_INTERLACE) ? "IL" : "",
           (mode->cable_type == CT_VGA) ? "VGA" : (mode->flags & VID_PAL) ? "PAL" : "NTSC",
           mode->fb_count);

    /* Pixelformat */
    data = (mode->pm << 2);

    if(ct == CT_VGA) {
        data |= 1 << 23;

        if(mode->flags & VID_LINEDOUBLE)
            data |= 2;
    }

    PVR_SET(PVR_FB_CFG_1, data);
    PVR_SET(PVR_FB_CFG_2, vid_bpp_to_pvr_cfg2[mode->pm]);

    /* Linestride */
    PVR_SET(PVR_RENDER_MODULO, (mode->width * vid_pmode_bpp[mode->pm]) / 8);

    /* Display size */
    data = ((mode->width * vid_pmode_bpp[mode->pm]) / 4) - 1;

    if(ct == CT_VGA || (!(mode->flags & VID_INTERLACE))) {
        data |= (1 << 20) | ((mode->height - 1) << 10);
    }
    else {
        data |= (((mode->width * vid_pmode_bpp[mode->pm] >> 2) + 1) << 20)
                | (((mode->height / 2) - 1) << 10);
    }

    PVR_SET(PVR_FB_SIZE, data);

    /* vblank irq */
    if(ct == CT_VGA) {
        PVR_SET(PVR_VPOS_IRQ, (mode->scanint1 << 16) | (mode->scanint2 << 1));
    }
    else {
        PVR_SET(PVR_VPOS_IRQ, (mode->scanint1 << 16) | mode->scanint2);
    }

    /* Interlace stuff */
    data = 0x100;

    if(mode->flags & VID_INTERLACE) {
        data |= 0x10;

        if(mode->flags & VID_PAL) {
            data |= 0x80;
        }
        else {
            data |= 0x40;
        }
    }

    PVR_SET(PVR_IL_CFG, data);

    /* Border window */
    PVR_SET(PVR_BORDER_X, (mode->borderx1 << 16) | mode->borderx2);
    PVR_SET(PVR_BORDER_Y, (mode->bordery1 << 16) | mode->bordery2);

    /* Scanlines and clocks. */
    PVR_SET(PVR_SCAN_CLK, (mode->scanlines << 16) | mode->clocks);

    /* Horizontal pixel doubling */
    if(mode->flags & VID_PIXELDOUBLE) {
        PVR_SET(PVR_VIDEO_CFG, PVR_GET(PVR_VIDEO_CFG) | 0x100);
    }
    else {
        PVR_SET(PVR_VIDEO_CFG, PVR_GET(PVR_VIDEO_CFG) & ~0x100);
    }

    /* Bitmap window */
    PVR_SET(PVR_BITMAP_X, mode->bitmapx);

    /* The upper 16 bits map to field-2 and need to be one more for PAL */
    if(mode->flags & VID_PAL) {
        PVR_SET(PVR_BITMAP_Y, ((mode->bitmapy + 1) << 16) | mode->bitmapy);
    }
    else {
        PVR_SET(PVR_BITMAP_Y, (mode->bitmapy << 16) | mode->bitmapy);
    }

    /* Everything is ok */
    memcpy(&currmode, mode, sizeof(vid_mode_t));
    vid_mode = &currmode;

    /* Set up the framebuffer */
    vid_mode->fb_curr = ~0;
    vid_flip(0);

    /* Set cable type */
    *((vuint32*)0xa0702c00) = (*((vuint32*)0xa0702c00) & 0xfffffcff) |
        ((ct & 3) << 8);

    /* Re-enable the display */
    vid_set_enabled(true);
}

/*-----------------------------------------------------------------------------*/
void vid_set_vram(uint32_t base) {
    vram_s = (uint16_t*)(PVR_RAM_BASE | base);
    vram_l = (uint32_t*)(PVR_RAM_BASE | base);
}

void vid_set_start(uint32_t base) {
    /* Set vram base of current framebuffer */
    base &= (PVR_RAM_SIZE - 1);
    PVR_SET(PVR_FB_ADDR, base);

    vid_set_vram(base);

    /* Set odd-field if interlaced. */
    if(vid_mode->flags & VID_INTERLACE) {
        PVR_SET(PVR_FB_IL_ADDR, base + (vid_mode->width * vid_pmode_bpp[vid_mode->pm]));
    }
}

uint32_t vid_get_start(int32_t fb) {
    /* If out of bounds, return current fb addr */
    if((fb < 0) || (fb >= vid_mode->fb_count)) {
        fb = vid_mode->fb_curr;
    }

    return (vid_mode->fb_size * fb);
}

/*-----------------------------------------------------------------------------*/
void vid_set_fb(int32_t fb) {
    uint16_t oldfb = vid_mode->fb_curr;

    if((fb < 0) || (fb >= vid_mode->fb_count)) {
        vid_mode->fb_curr++;
    }
    else {
        vid_mode->fb_curr = fb;
    }

    /* Roll over */
    vid_mode->fb_curr = vid_mode->fb_curr % vid_mode->fb_count;

    if(vid_mode->fb_curr == oldfb) {
        return;
    }

    vid_set_start(vid_get_start(vid_mode->fb_curr));
}

/*-----------------------------------------------------------------------------*/
void vid_flip(int32_t fb) {
    uint32_t base;

    vid_set_fb(fb);

    /* Set the vram_* pointers to the next fb */
    base = vid_get_start(((vid_mode->fb_curr + 1) % vid_mode->fb_count));
    vid_set_vram(base);
}

/*-----------------------------------------------------------------------------*/
uint32_t vid_border_color(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t obc = PVR_GET(PVR_BORDER_COLOR);
    PVR_SET(PVR_BORDER_COLOR, ((r & 0xFF) << 16) |
                       ((g & 0xFF) << 8) |
                       (b & 0xFF));
    return obc;
}

/*-----------------------------------------------------------------------------*/
/* Clears the screen with a given color */
void vid_clear(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t pixel16;
    uint32_t pixel32;

    switch(vid_mode->pm) {
        case PM_RGB555:
            pixel16 = ((r >> 3) << 10)
                      | ((g >> 3) << 5)
                      | ((b >> 3) << 0);
            sq_set16(vram_s, pixel16, (vid_mode->width * vid_mode->height) * vid_pmode_bpp[PM_RGB555]);
            break;
        case PM_RGB565:
            pixel16 = ((r >> 3) << 11)
                      | ((g >> 2) << 5)
                      | ((b >> 3) << 0);
            sq_set16(vram_s, pixel16, (vid_mode->width * vid_mode->height) * vid_pmode_bpp[PM_RGB565]);
            break;
        case PM_RGB888P:
            /* Need to come up with some way to fill this quickly. */
            dbglog(DBG_WARNING, "vid_clear: PM_RGB888P not supported, clearing with 0\n");
            sq_set32(vram_l, 0, (vid_mode->width * vid_mode->height) * vid_pmode_bpp[PM_RGB888P]);
            break;
        case PM_RGB0888:
            pixel32 = (r << 16) | (g << 8) | (b << 0);
            sq_set32(vram_l, pixel32, (vid_mode->width * vid_mode->height) * vid_pmode_bpp[PM_RGB0888]);
            break;
        default:
            dbglog(DBG_ERROR, "vid_clear: Invalid Pixel Mode: %i\n", vid_mode->pm);
            break;
    }
}

/*-----------------------------------------------------------------------------*/
/* Clears all of video memory as quickly as possible */
void vid_empty(void) {
    sq_clr((uint32_t *)PVR_RAM_BASE, PVR_RAM_SIZE);
}

/*-----------------------------------------------------------------------------*/
bool vid_get_enabled(void) {
    if(PVR_GET(PVR_FB_CFG_1) & 1) return true;
    else return false;
}

void vid_set_enabled(bool val) {
    /* If it's already the current setting, dont' do anything */
    if(val == vid_get_enabled()) return;

    if(val) {
        /* Re-enable the display */
        PVR_SET(PVR_VIDEO_CFG, PVR_GET(PVR_VIDEO_CFG) & ~8);
        PVR_SET(PVR_FB_CFG_1, PVR_GET(PVR_FB_CFG_1) | 1);
    }
    else {
        /* Blank screen and reset display enable (looks nicer) */
        PVR_SET(PVR_VIDEO_CFG, PVR_GET(PVR_VIDEO_CFG) | 8);    /* Blank */
        PVR_SET(PVR_FB_CFG_1, PVR_GET(PVR_FB_CFG_1) & ~1);     /* Display disable */
    }
}

/*-----------------------------------------------------------------------------*/
/* Waits for a vertical refresh to start. This is the period between
   when the scan beam reaches the bottom of the picture, and when it
   starts again at the top.

   Thanks to HeroZero for this info.

*/
void vid_waitvbl(void) {
    while(!(PVR_GET(PVR_SYNC_STATUS) & 0x01ff))
        ;

    while(PVR_GET(PVR_SYNC_STATUS) & 0x01ff)
        ;
}

/*-----------------------------------------------------------------------------*/
void vid_init(int disp_mode, vid_pixel_mode_t pixel_mode) {
    /* Set mode and clear vram */
    vid_set_mode(disp_mode, pixel_mode);
    vid_empty();
}

/*-----------------------------------------------------------------------------*/
void vid_shutdown(void) {
    /* Reset back to default mode, in case we're going back to a loader. */
    vid_init(DM_640x480, PM_RGB565);
}

void vid_set_dithering(bool enable) {
    uint32_t cfg = vid_bpp_to_pvr_cfg2[currmode.pm];

    if(enable)
        cfg |= PVR_PM_DITHER;
    else
        cfg &= ~PVR_PM_DITHER;

    PVR_SET(PVR_FB_CFG_2, cfg);
}
