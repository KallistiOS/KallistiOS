/* KallistiOS ##version##

   biosfont.c

   Copyright (C) 2000-2002 Megan Potter
   Japanese code Copyright (C) Kazuaki Matsumoto
   Copyright (C) 2017, 2024 Donald Haase
   Copyright (C) 2024 Andy Barajas
   Copyright (C) 2024 Falco Girgis
*/

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <dc/video.h>
#include <dc/biosfont.h>
#include <dc/syscalls.h>

#include <kos/dbglog.h>
#include <kos/thread.h>

/*

This module handles interfacing to the bios font. It supports the standard
European encodings via ISO8859-1, and Japanese in both Shift-JIS and EUC
modes. For Windows/Cygwin users, you'll probably want to call
bfont_set_encoding(BFONT_CODE_SJIS) so that your messages are displayed
properly; otherwise it will default to EUC (for *nix).

Thanks to Marcus Comstedt for the bios font information.

All the Japanese code is by Kazuaki Matsumoto.

Foreground/background color switching based on code by Chilly Willy.

Expansion to 4 and 8 bpp by Donald Haase.

*/

/* Our current conversion mode */
static uint8_t bfont_code_mode = BFONT_CODE_ISO8859_1;

/* Current colors/pixel format. Default to white foreground, black background
   and 16-bit drawing, so the default behavior doesn't change from what it has
   been forever. */
static uint32_t bfont_fgcolor = 0xFFFFFFFF;
static uint32_t bfont_bgcolor = 0x00000000;

static uint8_t *font_address = NULL;

static uint8_t *get_font_address(void) {
    if(!font_address)
        font_address = syscall_font_address();

    return font_address;
}

static inline uint8_t bits_per_pixel() {
    return ((vid_mode->pm == PM_RGB0888) ? sizeof(uint32_t) : sizeof(uint16_t)) << 3;
}

/* Select an encoding for Japanese (or disable) */
void bfont_set_encoding(bfont_code_t enc) {
    if(enc <= BFONT_CODE_RAW)
        bfont_code_mode = enc;
    else
        assert_msg(0, "Unknown bfont encoding mode");
}

/* Set the foreground color and return the old color */
uint32_t bfont_set_foreground_color(uint32_t c) {
    uint32_t rv = bfont_fgcolor;
    bfont_fgcolor = c;
    return rv;
}

/* Set the background color and return the old color */
uint32_t bfont_set_background_color(uint32_t c) {
    uint32_t rv = bfont_bgcolor;
    bfont_bgcolor = c;
    return rv;
}

int lock_bfont(void) {
    /* Just make sure no outside system took the lock */
    while(syscall_font_lock() != 0)
        thd_pass();

    return 0;
}

int unlock_bfont(void) {
    syscall_font_unlock();

    return 0;
}

/* Shift-JIS -> JIS conversion */
static uint32_t sjis2jis(uint32_t sjis) {
    unsigned int hib, lob;

    hib = (sjis >> 8) & 0xff;
    lob = sjis & 0xff;
    hib -= (hib <= 0x9f) ? 0x71 : 0xb1;
    hib = (hib << 1) + 1;

    if(lob > 0x7f) lob--;

    if(lob >= 0x9e) {
        lob -= 0x7d;
        hib++;
    }
    else
        lob -= 0x1f;

    return (hib << 8) | lob;
}


/* EUC -> JIS conversion */
static uint32_t euc2jis(uint32_t euc) {
    return euc & ~0x8080;
}

/* Given an ASCII character, find it in the BIOS font if possible */
uint8_t *bfont_find_char(uint32_t ch) {
    uint8_t *fa = get_font_address();
    /* By default, map to a space */
    uint32_t index = 72 << 2;

    /* 33-126 in ASCII are 1-94 in the font */
    if(ch >= 33 && ch <= 126)
        index = ch - 32;

    /* 160-255 in ASCII are 96-161 in the font */
    else if(ch >= 160 && ch <= 255)
        index = ch - (160 - 96);

    return fa + index * (BFONT_THIN_WIDTH*BFONT_HEIGHT/8);
}

/* JIS -> (kuten) -> address conversion */
uint8_t *bfont_find_char_jp(uint32_t ch) {
    uint8_t *fa = get_font_address();
    uint32_t ku, ten, kuten = 0;

    /* Do the requested code conversion */
    switch(bfont_code_mode) {
        case BFONT_CODE_ISO8859_1:
            return NULL;
        case BFONT_CODE_EUC:
            ch = euc2jis(ch);
            break;
        case BFONT_CODE_SJIS:
            ch = sjis2jis(ch);
            break;
        default:
            assert_msg(0, "Unknown bfont encoding mode");
    }

    if(ch > 0) {
        ku = ((ch >> 8) & 0x7F);
        ten = (ch & 0x7F);

        if(ku >= 0x30)
            ku -= 0x30 - 0x28;

        kuten = (ku - 0x21) * 94 + ten - 0x21;
    }

    return fa + (kuten + 144) * (BFONT_WIDE_WIDTH*BFONT_HEIGHT/8);
}


/* Half-width kana -> address conversion */
uint8_t *bfont_find_char_jp_half(uint32_t ch) {
    uint8_t *fa = get_font_address();
    return fa + (32 + ch) * (BFONT_THIN_WIDTH*BFONT_HEIGHT/8);
}

/* Draws one half-width row of a character to an output buffer of bit depth in bits per pixel */
static uint16_t *bfont_draw_one_row(uint16_t *b, uint16_t word, bool opaque, uint32_t fg, uint32_t bg, uint8_t bpp) {
    uint8_t x;
    uint32_t color = 0x0000;
    uint16_t write16 = 0x0000;
    uint16_t oldcolor = *b;

    if((bpp == 4)||(bpp == 8)) {
        /* For 4 or 8bpp we have to go 2 or 4 pixels at a time to properly write out in all cases. */
        uint8_t bMask = (bpp==4) ? 0xf : 0xff;
        uint8_t pix = 16/bpp;
        for(x = 0; x < BFONT_THIN_WIDTH; x++) {
            if(x%pix == 0) {
                oldcolor = *b;
                write16 = 0x0000;
            }

            if(word & (0x0800 >> x)) write16 |= fg<<(bpp*(x%pix));
            else {
                if(opaque)           write16 |= bg<<(bpp*(x%pix));
                else                 write16 |= oldcolor&(bMask<<(bpp*(x%pix)));
            }
            if(x%pix == (pix-1)) *b++ = write16;
        }
    }
    else {/* 16 or 32 */

        for(x = 0; x < BFONT_THIN_WIDTH; x++, b++) {
            if(word & (0x0800 >> x))
                color = fg;
            else {
                if(opaque)           color = bg;
                else                 continue;
            }
            if(bpp==16) *b = color & 0xffff;
            else if(bpp == 32) {*(uint32_t *)b = color; b++;}
        }
    }

    return b;
}

size_t bfont_draw_ex(void *buf, uint32_t bufwidth, uint32_t fg, uint32_t bg, 
                     uint8_t bpp, bool opaque, uint32_t c, bool wide, bool iskana) {
    uint8_t *ch;
    uint16_t word;
    uint8_t y;
    uint8_t *buffer = (uint8_t *)buf;

    /* If they're requesting a wide char and in the wrong format, kick this out */
    if(wide && (bfont_code_mode == BFONT_CODE_ISO8859_1)) {
        dbglog(DBG_ERROR, "bfont_draw_ex: can't draw wide in bfont mode %d\n", bfont_code_mode);
        return 0;
    }

    /* Just making sure we can draw the character we want to */
    if(bufwidth < (uint32_t)(BFONT_THIN_WIDTH*(wide+1))) {
        dbglog(DBG_ERROR, "bfont_draw_ex: buffer is too small to draw into\n");
        return 0;
    }

    if(lock_bfont() < 0) {
        dbglog(DBG_ERROR, "bfont_draw_ex: error requesting font access\n");
        return 0;
    }

    /* Translate the character */
    if(bfont_code_mode == BFONT_CODE_RAW)
        ch = get_font_address() + c;
    else if(wide && ((bfont_code_mode == BFONT_CODE_EUC) || (bfont_code_mode == BFONT_CODE_SJIS)))
        ch = bfont_find_char_jp(c);
    else {
        if(iskana)
            ch = bfont_find_char_jp_half(c);
        else
            ch = bfont_find_char(c);
    }

    /* Increment over the height of the font. 3bytes at a time (2 thin or 1 wide row) */
    for(y = 0; y < BFONT_HEIGHT; y+= (2-wide),ch+=((BFONT_THIN_WIDTH*2)/8)) {
        /* Do the first row, or half row */
        word = (((uint16_t)ch[0]) << 4) | ((ch[1] >> 4) & 0x0f);
        buffer = (uint8_t *)bfont_draw_one_row((uint16_t *)buffer, word, opaque, fg, bg, bpp);

        /* If we're thin, increment to next row, otherwise continue the row */
        if(!wide) buffer += ((bufwidth - BFONT_THIN_WIDTH)*bpp)/8;

        /* Do the second row, or second half */
        word = ((((uint16_t)ch[1]) << 8) & 0xf00) | ch[2];

        buffer = (uint8_t *)bfont_draw_one_row((uint16_t *)buffer, word, opaque, fg, bg, bpp);

        /* Increment to the next row. */
        if(!wide) buffer += ((bufwidth - BFONT_THIN_WIDTH)*bpp)/8;
        else buffer += ((bufwidth - BFONT_WIDE_WIDTH)*bpp)/8;
    }

    if(unlock_bfont() < 0)
        dbglog(DBG_ERROR, "bfont_draw_ex: error releasing font access\n");

    /* Return the horizontal distance covered in bytes */
    if(wide)
        return (BFONT_WIDE_WIDTH*bpp)/8;
    else
        return (BFONT_THIN_WIDTH*bpp)/8;
}

/* Draw half-width kana */
size_t bfont_draw_thin(void *b, uint32_t bufwidth, bool opaque, uint32_t c, bool iskana) {
    return bfont_draw_ex(b, bufwidth, bfont_fgcolor, bfont_bgcolor, 
                         bits_per_pixel(), opaque, c, false, iskana);
}

/* Compat function */
size_t bfont_draw(void *buffer, uint32_t bufwidth, bool opaque, uint32_t c) {
    return bfont_draw_ex(buffer, bufwidth, bfont_fgcolor, bfont_bgcolor, 
                        bits_per_pixel(), opaque, c, false, false);
}

/* Draw wide character */
size_t bfont_draw_wide(void *b, uint32_t bufwidth, bool opaque, uint32_t c) {
    return bfont_draw_ex(b, bufwidth, bfont_fgcolor, bfont_bgcolor, 
                         bits_per_pixel(), opaque, c, true, false);
}

void bfont_draw_str_ex(void *b, uint32_t width, uint32_t fg, uint32_t bg, 
                       uint8_t bpp, bool opaque, const char *str) {
    bool wideChr;
    uint16_t nChr, nMask;
    uint32_t line_start = 0;
    uint8_t *buffer = (uint8_t *)b;

    while(*str) {
        wideChr = false;
        nChr = *str & 0xff;

        if(nChr == '\n') {
            /* Move to the beginning of the next line */
            buffer = (uint8_t *)b + line_start + (width * BFONT_HEIGHT * (bpp / 8));
            line_start = buffer - (uint8_t *)b;
            str++;
            continue;
        }
        else if(nChr == '\t') {
            /* Draw four spaces on the current line */
            if(opaque) {
                nChr = bfont_code_mode == BFONT_CODE_ISO8859_1 ? 0x20 : 0xa0;
                buffer += bfont_draw_ex(buffer, width, fg, bg, bpp, opaque, nChr, false, false);
                buffer += bfont_draw_ex(buffer, width, fg, bg, bpp, opaque, nChr, false, false);
                buffer += bfont_draw_ex(buffer, width, fg, bg, bpp, opaque, nChr, false, false);
                buffer += bfont_draw_ex(buffer, width, fg, bg, bpp, opaque, nChr, false, false);
            }
            else /* Spaces are always thin width characters */
                buffer += (4 * ((BFONT_THIN_WIDTH * bpp)/8));
            
            str++;
            continue;
        }

        /* Non-western, non-ASCII character */
        if(bfont_code_mode != BFONT_CODE_ISO8859_1 && (nChr & 0x80)) {
            switch(bfont_code_mode) {
                case BFONT_CODE_EUC:

                    /* Check if the character is the 'SS2' character in EUC-JP */
                    if(nChr == 0x8e) {
                        str++;
                        nChr = *str & 0xff;

                        /* Is a valid half-width katakana character? */
                        if((nChr < 0xa1) || (nChr > 0xdf))
                            nChr = 0xa0;    /* Blank Space */
                    }
                    else
                        wideChr = true;

                    break;
                case BFONT_CODE_SJIS:
                    nMask = nChr & 0xf0;

                    /* Check if the character is part of the valid Shift ranges */
                    if((nMask == 0x80) || (nMask == 0x90) || (nMask == 0xe0))
                        wideChr = true;

                    break;
                default:
                    assert_msg(0, "Unknown bfont encoding mode");
            }

            if(wideChr) {
                str++;
                nChr = (nChr << 8) | (*str & 0xff);
                buffer += bfont_draw_ex(buffer, width, fg, bg, bpp, opaque, nChr, true, false);
            }
            else
                buffer += bfont_draw_ex(buffer, width, fg, bg, bpp, opaque, nChr, false, true);
        }
        else
            buffer += bfont_draw_ex(buffer, width, fg, bg, bpp, opaque, nChr, false, false);

        str++;
    }
}

void bfont_draw_str_ex_vfmt(void *b, uint32_t width, uint32_t fg, uint32_t bg,
                            uint8_t bpp, bool opaque, const char *fmt,
                            va_list *var_args) {
    /* Maximum of 1060 thin characters onscreen, plus padding for multiple of 32. */
    char string[1088];

    vsnprintf(string, sizeof(string), fmt, *var_args);
    bfont_draw_str_ex(b, width, fg, bg, bpp, opaque, string);
}

/* Draw string of full-width (wide) and half-width (thin) characters
   Note that this handles the case of mixed encodings unless Japanese
   support is disabled (BFONT_CODE_ISO8859_1).
*/
void bfont_draw_str_ex_fmt(void *b, uint32_t width, uint32_t fg, uint32_t bg, uint8_t bpp,
                           bool opaque, const char *fmt, ...) {
    va_list var_args;
    va_start(var_args, fmt);

    bfont_draw_str_ex_vfmt(b, width, fg, bg, bpp, opaque, fmt, &var_args);

    va_end(var_args);
}

void bfont_draw_str(void *b, uint32_t width, bool opaque, const char *str) {
    bfont_draw_str_ex(b, width, bfont_fgcolor, bfont_bgcolor,
                     bits_per_pixel(), opaque, str);
}

void bfont_draw_str_fmt(void *b, uint32_t width, bool opaque, const char *fmt,
                        ...) {
    va_list var_args;
    va_start(var_args, fmt);

    bfont_draw_str_ex_vfmt(b, width, bfont_fgcolor, bfont_bgcolor,
                           bits_per_pixel(), opaque, fmt, &var_args);

    va_end(var_args);
}

void bfont_draw_str_vram_vfmt(uint32_t x, uint32_t y, uint32_t fg, 
                              uint32_t bg, bool opaque, const char *fmt, 
                              va_list *var_args) {
    uint32_t bpp = bits_per_pixel();
    void *vram = vram_s;
    uint32_t offset = (y * vid_mode->width + x);

    if(bpp == 16)
        vram = (uint16_t *)vram + offset;
    else if(bpp == 32)
        vram = (uint32_t *)vram + offset;

    bfont_draw_str_ex_vfmt(vram, vid_mode->width, fg, bg, bpp, opaque, fmt,
                           var_args);
}

void bfont_draw_str_vram_fmt(uint32_t x, uint32_t y, bool opaque, 
                            const char *fmt, ...) {
    va_list var_args;
    va_start(var_args, fmt);
    
    bfont_draw_str_vram_vfmt(x, y, bfont_fgcolor, bfont_bgcolor, opaque, fmt, 
                            &var_args);

    va_end(var_args);
}

uint8_t *bfont_find_icon(bfont_vmu_icon_t icon) {
    if(icon > BFONT_ICON_EMBROIDERY)
        return NULL;

    int icon_offset = BFONT_VMU_DREAMCAST_SPECIFIC +
        (icon * BFONT_ICON_DIMEN * BFONT_ICON_DIMEN / 8);
    uint8_t *fa = get_font_address();

    return fa + icon_offset;
}
