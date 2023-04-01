/* KallistiOS ##version##

   video.c

   (c)2001 Anders Clerwall (scav)
   Parts (c)2000-2001 Megan Potter
 */

#include <dc/video.h>
#include <dc/sq.h>
#include <string.h>
#include <stdio.h>

#define FBPOS(n) (n * 0x200000)

/*-----------------------------------------------------------------------------*/
/* This table is indexed w/ DM_* */
vid_mode_t vid_builtin[DM_MODE_COUNT] = {
    /* NULL mode.. */
    /* DM_INVALID = 0 */
    { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, { 0, 0, 0, 0 } },

    /* 320x240 VGA 60Hz */
    /* DM_320x240_VGA */
    {
        DM_320x240,
        320, 240,
        VID_PIXELDOUBLE | VID_LINEDOUBLE,
        CT_VGA,
        0,
        262, 857,
        0xAC, 0x28,
        0x15, 0x104,
        141, 843,
        24, 263,
        0, 1,
        { 0, 0, 0, 0 }
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
        0xA4, 0x18,
        0x15, 0x104,
        141, 843,
        24, 263,
        0, 1,
        { 0, 0, 0, 0 }
    },

    /* 640x480 VGA 60Hz */
    /* DM_640x480_VGA */
    {
        DM_640x480,
        640, 480,
        VID_INTERLACE,
        CT_VGA,
        0,
        0x20C, 0x359,
        0xAC, 0x28,
        0x15, 0x104,
        0x7E, 0x345,
        0x24, 0x204,
        0, 1,
        { 0, 0, 0, 0 }
    },

    /* 640x480 NTSC 60Hz IL */
    /* DM_640x480_NTSC_IL */
    {
        DM_640x480,
        640, 480,
        VID_INTERLACE,
        CT_ANY,
        0,
        0x20C, 0x359,
        0xA4, 0x12,
        0x15, 0x104,
        0x7E, 0x345,
        0x24, 0x204,
        0, 1,
        { 0, 0, 0, 0 }
    },

    /* 800x608 NTSC 60Hz (VGA) [BROKEN!] */
    /* DM_800x608_VGA */
    {
        DM_800x608,
        320, 240,
        VID_INTERLACE,
        1/*CT_ANY*/, /* This will block the mode from being set. */
        0,
        262, 857,
        164, 24,
        21, 82,
        141, 843,
        24, 264,
        0, 1,
        { 0, 0, 0, 0 }
    },

    /* 640x480 PAL 50Hz IL */
    /* DM_640x480_PAL_IL */
    {
        DM_640x480,
        640, 480,
        VID_INTERLACE | VID_PAL,
        CT_ANY,
        0,
        0x270, 0x35F,
        0xAE, 0x2D,
        0x15, 0x104,
        0x8D, 0x34B,
        0x2C, 0x26C,
        0, 1,
        { 0, 0, 0, 0 }
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
        0x15, 0x104,
        0x8D, 0x34B,
        0x2C, 0x26C,
        0, 1,
        { 0, 0, 0, 0 }
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
        0x15, 0x104,
        0x2e, 0x345,
        0x24, 0x204,
        0, 1,
        { 0, 0, 0, 0 }
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
        0x18, 0x104,
        0x36, 0x34b,
        0x2c, 0x26c,
        0, 1,
        { 0, 0, 0, 0 }
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
        0x18, 0x104,
        0x36, 0x34b,
        0x2c, 0x26c,
        0, 1,
        { 0, 0, 0, 0 }
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
        0, 1,
        { 0, 0, 0, 0 }
    },

    /* All of the modes below this comment are exactly the same as the ones
       above, other than that they support multiple framebuffers (in the current
       case, 4). They're only particularly useful if you're doing a drawing by
       directly writing to the framebuffer, and are not useful at all if you're
       using the PVR to do your drawing. */
    /* 320x240 VGA 60Hz */
    /* DM_320x240_VGA_MB */
    {
        DM_320x240 | DM_MULTIBUFFER,
        320, 240,
        VID_PIXELDOUBLE | VID_LINEDOUBLE,
        CT_VGA,
        0,
        262, 857,
        0xAC, 0x28,
        0x15, 0x104,
        141, 843,
        24, 263,
        0, VID_MAX_FB,
        { FBPOS(0), FBPOS(1), FBPOS(2), FBPOS(3) }
    },

    /* 320x240 NTSC 60Hz */
    /* DM_320x240_NTSC_MB */
    {
        DM_320x240 | DM_MULTIBUFFER,
        320, 240,
        VID_PIXELDOUBLE | VID_LINEDOUBLE,
        CT_ANY,
        0,
        262, 857,
        0xA4, 0x18,
        0x15, 0x104,
        141, 843,
        24, 263,
        0, VID_MAX_FB,
        { FBPOS(0), FBPOS(1), FBPOS(2), FBPOS(3) }
    },

    /* 640x480 VGA 60Hz */
    /* DM_640x480_VGA_MB */
    {
        DM_640x480 | DM_MULTIBUFFER,
        640, 480,
        VID_INTERLACE,
        CT_VGA,
        0,
        0x20C, 0x359,
        0xAC, 0x28,
        0x15, 0x104,
        0x7E, 0x345,
        0x24, 0x204,
        0, VID_MAX_FB,
        { FBPOS(0), FBPOS(1), FBPOS(2), FBPOS(3) }
    },

    /* 640x480 NTSC 60Hz IL */
    /* DM_640x480_NTSC_IL_MB */
    {
        DM_640x480 | DM_MULTIBUFFER,
        640, 480,
        VID_INTERLACE,
        CT_ANY,
        0,
        0x20C, 0x359,
        0xA4, 0x12,
        0x15, 0x104,
        0x7E, 0x345,
        0x24, 0x204,
        0, VID_MAX_FB,
        { FBPOS(0), FBPOS(1), FBPOS(2), FBPOS(3) }
    },

    /* 800x608 NTSC 60Hz (VGA) [BROKEN!] */
    /* DM_800x608_VGA_MB */
    {
        DM_800x608 | DM_MULTIBUFFER,
        320, 240,
        VID_INTERLACE,
        1/*CT_ANY*/, /* This will block the mode from being set. */
        0,
        262, 857,
        164, 24,
        21, 82,
        141, 843,
        24, 264,
        0, VID_MAX_FB,
        { FBPOS(0), FBPOS(1), FBPOS(2), FBPOS(3) }
    },

    /* 640x480 PAL 50Hz IL */
    /* DM_640x480_PAL_IL_MB */
    {
        DM_640x480 | DM_MULTIBUFFER,
        640, 480,
        VID_INTERLACE | VID_PAL,
        CT_ANY,
        0,
        0x270, 0x35F,
        0xAE, 0x2D,
        0x15, 0x104,
        0x8D, 0x34B,
        0x2C, 0x26C,
        0, VID_MAX_FB,
        { FBPOS(0), FBPOS(1), FBPOS(2), FBPOS(3) }
    },

    /* 256x256 PAL 50Hz IL (seems to output the same w/o VID_PAL, ie. in NTSC IL mode) */
    /* DM_256x256_PAL_IL_MB */
    {
        DM_256x256 | DM_MULTIBUFFER,
        256, 256,
        VID_PIXELDOUBLE | VID_LINEDOUBLE | VID_INTERLACE | VID_PAL,
        CT_ANY,
        0,
        624, 863,
        226, 37,
        0x15, 0x104,
        0x8D, 0x34B,
        0x2C, 0x26C,
        0, VID_MAX_FB,
        { FBPOS(0), FBPOS(1), FBPOS(2), FBPOS(3) }
    },

    /* 768x480 NTSC 60Hz IL (thanks DCGrendel) */
    /* DM_768x480_NTSC_IL_MB */
    {
        DM_768x480 | DM_MULTIBUFFER,
        768, 480,
        VID_INTERLACE,
        CT_ANY,
        0,
        524, 857,
        96, 18,
        0x15, 0x104,
        0x2e, 0x345,
        0x24, 0x204,
        0, VID_MAX_FB,
        { FBPOS(0), FBPOS(1), FBPOS(2), FBPOS(3) }
    },

    /* 768x576 PAL 50Hz IL (DCG) */
    /* DM_768x576_PAL_IL_MB */
    {
        DM_768x576 | DM_MULTIBUFFER,
        768, 576,
        VID_INTERLACE | VID_PAL,
        CT_ANY,
        0,
        624, 863,
        88, 16,
        0x18, 0x104,
        0x36, 0x34b,
        0x2c, 0x26c,
        0, VID_MAX_FB,
        { FBPOS(0), FBPOS(1), FBPOS(2), FBPOS(3) }
    },

    /* 768x480 PAL 50Hz IL */
    /* DM_768x480_PAL_IL_MB */
    {
        DM_768x480 | DM_MULTIBUFFER,
        768, 480,
        VID_INTERLACE | VID_PAL,
        CT_ANY,
        0,
        624, 863,
        88, 16,
        0x18, 0x104,
        0x36, 0x34b,
        0x2c, 0x26c,
        0, VID_MAX_FB,
        { FBPOS(0), FBPOS(1), FBPOS(2), FBPOS(3) }
    },

    /* 320x240 PAL 50Hz (thanks Marco Martins aka Mekanaizer) */
    /* DM_320x240_PAL_MB */
    {
        DM_320x240 | DM_MULTIBUFFER,
        320, 240,
        VID_PIXELDOUBLE | VID_LINEDOUBLE | VID_PAL,
        CT_ANY,
        0,
        312, 863,
        174, 45,
        21, 260,
        141, 843,
        44, 620,
        0, VID_MAX_FB,
        { FBPOS(0), FBPOS(1), FBPOS(2), FBPOS(3) }
    },

    /* END */
    /* DM_SENTINEL */
    { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, { 0, 0, 0, 0 } }

    /* DM_MODE_COUNT */
};

/*-----------------------------------------------------------------------------*/
static vuint32 *regs = (uint32*)0xA05F8000;
static vid_mode_t  currmode = { 0 };
vid_mode_t  *vid_mode = 0;
uint16      *vram_s;
uint32      *vram_l;

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
int vid_check_cable() {
#ifndef _arch_sub_naomi
    vuint32 * porta = (vuint32 *)0xff80002c;

    *porta = (*porta & 0xfff0ffff) | 0x000a0000;

    /* Read port8 and port9 */
    return (*((vuint16*)(porta + 1)) >> 8) & 3;
#else
    /* XXXX: This still needs to be figured out for NAOMI. For now, assume
       VGA mode. */
    return 0;
#endif
}

/*-----------------------------------------------------------------------------*/
void vid_set_mode(int dm, int pm) {
    vid_mode_t mode;
    int i, ct, found, dm2;

    ct = vid_check_cable();

    /* Remove the multi-buffering flag from the mode, if its present, and save
       the state of that flag. */
    dm2 = dm & DM_MULTIBUFFER;
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
            if(vid_builtin[i].generic != (dm | dm2))
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

    /* This is also to be generic */
    mode.cable_type = ct;

    /* This will make a private copy of our "mode" */
    vid_set_mode_ex(&mode);
}

/*-----------------------------------------------------------------------------*/
void vid_set_mode_ex(vid_mode_t *mode) {
    static uint8 bpp[4] = { 2, 2, 0, 4 };
    uint16 ct;
    uint32 data;


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
    regs[0x3A] |= 8;    /* Blank */
    regs[0x11] &= ~1;   /* Display disable */

    /* Clear interlace flag if VGA (this maybe should be in here?) */
    if(ct == CT_VGA) {
        mode->flags &= ~VID_INTERLACE;

        if(mode->flags & VID_LINEDOUBLE)
            mode->scanlines *= 2;
    }

    dbglog(DBG_INFO, "vid_set_mode: %ix%i%s %s%s\n", mode->width, mode->height,
           (mode->flags & VID_INTERLACE) ? "IL" : "",
           (mode->cable_type == CT_VGA) ? "VGA" : (mode->flags & VID_PAL) ? "PAL" : "NTSC",
           (mode->generic & DM_MULTIBUFFER) ? " multi-buffered" : "");

    vid_border_color(0, 0, 0);

    /* Pixelformat */
    data = (mode->pm << 2);

    if(ct == CT_VGA) {
        data |= 1 << 23;

        if(mode->flags & VID_LINEDOUBLE)
            data |= 2;
    }

    regs[0x11] = data;

    /* Linestride */
    regs[0x13] = (mode->width * bpp[mode->pm]) / 8;

    /* Display size */
    data = ((mode->width * bpp[mode->pm]) / 4) - 1;

    if(ct == CT_VGA || (!(mode->flags & VID_INTERLACE))) {
        data |= (1 << 20) | ((mode->height - 1) << 10);
    }
    else {
        data |= (((mode->width * bpp[mode->pm] >> 2) + 1) << 20)
                | (((mode->height / 2) - 1) << 10);
    }

    regs[0x17] = data;

    /* vblank irq */
    if(ct == CT_VGA) {
        regs[0x33] = (mode->scanint1 << 16) | (mode->scanint2 << 1);
    }
    else {
        regs[0x33] = (mode->scanint1 << 16) | mode->scanint2;
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

    regs[0x34] = data;

    /* Border window */
    regs[0x35] = (mode->borderx1 << 16) | mode->borderx2;
    regs[0x37] = (mode->bordery1 << 16) | mode->bordery2;

    /* Scanlines and clocks. */
    regs[0x36] = (mode->scanlines << 16) | mode->clocks;

    /* Horizontal pixel doubling */
    if(mode->flags & VID_PIXELDOUBLE) {
        regs[0x3A] |= 0x100;
    }
    else {
        regs[0x3A] &= ~0x100;
    }

    /* Bitmap window */
    regs[0x3B] = mode->bitmapx;
    data = mode->bitmapy;

    if(mode->flags & VID_PAL) {
        data++;
    }

    data = (data << 16) | mode->bitmapy;
    regs[0x3C] = data;

    /* Everything is ok */
    memcpy(&currmode, mode, sizeof(vid_mode_t));
    vid_mode = &currmode;

    /* Set up the framebuffer */
    vid_mode->fb_curr = -1;
    vid_flip(0);

    /* Set cable type */
    *((vuint32*)0xa0702c00) = (*((vuint32*)0xa0702c00) & 0xfffffcff) |
        ((ct & 3) << 8);

    /* Re-enable the display */
    regs[0x3A] &= ~8;
    regs[0x11] |= 1;
}

/*-----------------------------------------------------------------------------*/
void vid_set_start(uint32 base) {
    static uint8 bpp[4] = { 2, 2, 0, 4 };

    /* Set vram base of current framebuffer */
    base &= 0x007FFFFF;
    regs[0x14] = base;

    /* These are nice to have. */
    vram_s = (uint16*)(0xA5000000 | base);
    vram_l = (uint32*)(0xA5000000 | base);

    /* Set odd-field if interlaced. */
    if(vid_mode->flags & VID_INTERLACE) {
        regs[0x15] = base + (currmode.width * bpp[currmode.pm]);
    }
}

/*-----------------------------------------------------------------------------*/
void vid_flip(int fb) {
    uint16 oldfb;
    uint32 base;

    oldfb = vid_mode->fb_curr;

    if(fb < 0) {
        vid_mode->fb_curr++;
    }
    else {
        vid_mode->fb_curr = fb;
    }

    vid_mode->fb_curr &= (vid_mode->fb_count - 1);

    if(vid_mode->fb_curr == oldfb) {
        return;
    }

    vid_set_start(vid_mode->fb_base[vid_mode->fb_curr]);

    /* Set the vram_* pointers as expected */
    base = vid_mode->fb_base[(vid_mode->fb_curr + 1) & (vid_mode->fb_count - 1)];
    vram_s = (uint16*)(0xA5000000 | base);
    vram_l = (uint32*)(0xA5000000 | base);
}

/*-----------------------------------------------------------------------------*/
uint32 vid_border_color(int r, int g, int b) {
    uint32 obc = regs[0x0040 / 4];
    regs[0x0040 / 4] = ((r & 0xFF) << 16) |
                       ((g & 0xFF) << 8) |
                       (b & 0xFF);
    return obc;
}

/*-----------------------------------------------------------------------------*/
/* Clears the screen with a given color

    [This is the old KOS function by Megan.]
*/
void vid_clear(int r, int g, int b) {
    uint16 pixel16;
    uint32 pixel32;

    switch(vid_mode->pm) {
        case PM_RGB555:
            pixel16 = ((r >> 3) << 10)
                      | ((g >> 3) << 5)
                      | ((b >> 3) << 0);
            sq_set16(vram_s, pixel16, (vid_mode->width * vid_mode->height) * 2);
            break;
        case PM_RGB565:
            pixel16 = ((r >> 3) << 11)
                      | ((g >> 2) << 5)
                      | ((b >> 3) << 0);
            sq_set16(vram_s, pixel16, (vid_mode->width * vid_mode->height) * 2);
            break;
        case PM_RGB888:
            pixel32 = (r << 16) | (g << 8) | (b << 0);
            sq_set32(vram_l, pixel32, (vid_mode->width * vid_mode->height) * 4);
            break;
    }
}

/*-----------------------------------------------------------------------------*/
/* Clears all of video memory as quickly as possible

    [This is the old KOS function by Megan.]
*/
void vid_empty() {
    /* We'll use the actual base address here since the vram_* pointers
       can now move around */
    sq_clr((uint32 *)0xa5000000, 8 * 1024 * 1024);
}

/*-----------------------------------------------------------------------------*/
/* Waits for a vertical refresh to start. This is the period between
   when the scan beam reaches the bottom of the picture, and when it
   starts again at the top.

   Thanks to HeroZero for this info.

   [This is the old KOS function by Megan.]
*/
void vid_waitvbl() {
    vuint32 *vbl = regs + 0x010c / 4;

    while(!(*vbl & 0x01ff))
        ;

    while(*vbl & 0x01ff)
        ;
}

/*-----------------------------------------------------------------------------*/
void vid_init(int disp_mode, int pixel_mode) {
    /* Set mode and clear vram */
    vid_set_mode(disp_mode, pixel_mode);
    vid_clear(0, 0, 0);
    vid_flip(0);
}

/*-----------------------------------------------------------------------------*/
void vid_shutdown() {
    /* Play nice with loaders, like KOS used to do. */
    vid_init(DM_640x480, PM_RGB565);
}
