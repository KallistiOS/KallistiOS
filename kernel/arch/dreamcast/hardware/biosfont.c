/* KallistiOS ##version##

   biosfont.c

   Copyright (C) 2000-2002 Megan Potter
   Japanese code Copyright (C) Kazuaki Matsumoto
   Copyright (C) 2017, 2024 Donald Haase
   Copyright (C) 2024 Andy Barajas
*/

#include <assert.h>

#include <dc/biosfont.h>
#include <dc/syscalls.h>

#include <kos/dbglog.h>

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
static uint8 bfont_code_mode = BFONT_CODE_ISO8859_1;

/* Current colors/pixel format. Default to white foreground, black background
   and 16-bit drawing, so the default behavior doesn't change from what it has
   been forever. */
static uint32 bfont_fgcolor = 0xFFFFFFFF;
static uint32 bfont_bgcolor = 0x00000000;
static int bfont_32bit = 0;

/* Select an encoding for Japanese (or disable) */
void bfont_set_encoding(uint8 enc) {
    if(enc <= BFONT_CODE_RAW)
        bfont_code_mode = enc;
    else
        assert_msg(0, "Unknown bfont encoding mode");
}

/* Set the foreground color and return the old color */
uint32 bfont_set_foreground_color(uint32 c) {
    uint32 rv = bfont_fgcolor;
    bfont_fgcolor = c;
    return rv;
}

/* Set the background color and return the old color */
uint32 bfont_set_background_color(uint32 c) {
    uint32 rv = bfont_bgcolor;
    bfont_bgcolor = c;
    return rv;
}

/* Set the font to draw in 32 or 16 bit mode.
    Deprecated: This will only impact compat functions for now.
    Moving forward, the compat will be 16bit only. */
int bfont_set_32bit_mode(int on) {
    int rv = bfont_32bit;
    bfont_32bit = !!on;
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
uint32 sjis2jis(uint32 sjis) {
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
uint32 euc2jis(uint32 euc) {
    return euc & ~0x8080;
}

/* Given an ASCII character, find it in the BIOS font if possible */
uint8 *bfont_find_char(uint32 ch) {
    uint8   *fa = syscall_font_address();
    /* By default, map to a space */
    uint32 index = 72 << 2;

    /* 33-126 in ASCII are 1-94 in the font */
    if(ch >= 33 && ch <= 126)
        index = ch - 32;

    /* 160-255 in ASCII are 96-161 in the font */
    else if(ch >= 160 && ch <= 255)
        index = ch - (160 - 96);

    return fa + index * (BFONT_THIN_WIDTH*BFONT_HEIGHT/8);
}

/* JIS -> (kuten) -> address conversion */
uint8 *bfont_find_char_jp(uint32 ch) {
    uint8   *fa = syscall_font_address();
    uint32 ku, ten, kuten = 0;

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
uint8 *bfont_find_char_jp_half(uint32 ch) {
    uint8 *fa = syscall_font_address();
    return fa + (32 + ch) * (BFONT_THIN_WIDTH*BFONT_HEIGHT/8);
}

/* Draws one half-width row of a character to an output buffer of bit depth in bits per pixel */
uint16 *bfont_draw_one_row(uint16 *b, uint16 word, uint8 opaque, uint32 fg, uint32 bg, uint8 bpp) {
    uint8 x;
    uint32 color = 0x0000;
    uint16 write16 = 0x0000;
    uint16 oldcolor = *b;

    if((bpp == 4)||(bpp == 8)) {
        /* For 4 or 8bpp we have to go 2 or 4 pixels at a time to properly write out in all cases. */
        uint8 bMask = (bpp==4) ? 0xf : 0xff;
        uint8 pix = 16/bpp;
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
            else if(bpp == 32) {*(uint32 *)b = color; b++;}
        }
    }

    return b;
}

unsigned char bfont_draw_ex(uint8 *buffer, uint32 bufwidth, uint32 fg, uint32 bg, uint8 bpp, uint8 opaque, uint32 c, uint8 wide, uint8 iskana) {
    uint8 *ch;
    uint16 word;
    uint8 y;

    /* If they're requesting a wide char and in the wrong format, kick this out */
    if(wide && (bfont_code_mode == BFONT_CODE_ISO8859_1)) {
        dbglog(DBG_ERROR, "bfont_draw_ex: can't draw wide in bfont mode %d\n", bfont_code_mode);
        return 0;
    }

    /* Just making sure we can draw the character we want to */
    if(bufwidth < (uint32)(BFONT_THIN_WIDTH*(wide+1))) {
        dbglog(DBG_ERROR, "bfont_draw_ex: buffer is too small to draw into\n");
        return 0;
    }

    if(lock_bfont() < 0) {
        dbglog(DBG_ERROR, "bfont_draw_ex: error requesting font access\n");
        return 0;
    }

    /* Translate the character */
    if(bfont_code_mode == BFONT_CODE_RAW)
        ch = syscall_font_address() + c;
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
        word = (((uint16)ch[0]) << 4) | ((ch[1] >> 4) & 0x0f);
        buffer = (uint8*)bfont_draw_one_row((uint16*)buffer, word, opaque, fg, bg, bpp);

        /* If we're thin, increment to next row, otherwise continue the row */
        if(!wide) buffer += ((bufwidth - BFONT_THIN_WIDTH)*bpp)/8;

        /* Do the second row, or second half */
        word = ((((uint16)ch[1]) << 8) & 0xf00) | ch[2];

        buffer = (uint8*)bfont_draw_one_row((uint16*)buffer, word, opaque, fg, bg, bpp);

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
unsigned char bfont_draw_thin(void *b, uint32 bufwidth, uint8 opaque, uint32 c, uint8 iskana) {
    return bfont_draw_ex((uint8 *)b, bufwidth, bfont_fgcolor, bfont_bgcolor, (bfont_32bit ? (sizeof (uint32)) : (sizeof (uint16))) << 3, opaque, c, 0, iskana);
}

/* Compat function */
unsigned char bfont_draw(void *buffer, uint32 bufwidth, uint8 opaque, uint32 c) {
    return bfont_draw_ex((uint8 *)buffer, bufwidth, bfont_fgcolor, bfont_bgcolor, (bfont_32bit ? (sizeof (uint32)) : (sizeof (uint16))) << 3, opaque, c, 0, 0);
}

/* Draw wide character */
unsigned char bfont_draw_wide(void *b, uint32 bufwidth, uint8 opaque, uint32 c) {
    return bfont_draw_ex((uint8 *)b, bufwidth, bfont_fgcolor, bfont_bgcolor, (bfont_32bit ? (sizeof (uint32)) : (sizeof (uint16))) << 3, opaque, c, 1, 0);
}

/* Draw string of full-width (wide) and half-width (thin) characters
   Note that this handles the case of mixed encodings unless Japanese
   support is disabled (BFONT_CODE_ISO8859_1).
   XXX: Seems like this can be shrunk to use uint8 for nChr/Mask/Flag and
    getting rid of nMask.
   */
void bfont_draw_str_ex(void *b, uint32 width, uint32 fg, uint32 bg, uint8 bpp, uint8 opaque, const char *str) {
    uint16 nChr, nMask, nFlag;
    uint8 *buffer = (uint8 *)b;

    while(*str) {
        nFlag = 0;
        nChr = *str & 0xff;

        if(bfont_code_mode != BFONT_CODE_ISO8859_1 && (nChr & 0x80)) {
            switch(bfont_code_mode) {
                case BFONT_CODE_EUC:

                    if(nChr == 0x8e) {
                        str++;
                        nChr = *str & 0xff;

                        if((nChr < 0xa1) || (nChr > 0xdf))
                            nChr = 0xa0;    /* Blank Space */
                    }
                    else
                        nFlag = 1;

                    break;
                case BFONT_CODE_SJIS:
                    nMask = nChr & 0xf0;

                    if((nMask == 0x80) || (nMask == 0x90) || (nMask == 0xe0))
                        nFlag = 1;

                    break;
                default:
                    assert_msg(0, "Unknown bfont encoding mode");
            }

            if(nFlag == 1) {
                str++;
                nChr = (nChr << 8) | (*str & 0xff);
                buffer += bfont_draw_ex(buffer, width, fg, bg, bpp, opaque, nChr, 1, 0);
            }
            else
                buffer += bfont_draw_ex(buffer, width, fg, bg, bpp, opaque, nChr, 0, 1);
        }
        else
            buffer += bfont_draw_ex(buffer, width, fg, bg, bpp, opaque, nChr, 0, 0);

        str++;
    }
}

void bfont_draw_str(void *b, uint32 width, uint8 opaque, const char *str) {
    bfont_draw_str_ex(b, width, bfont_fgcolor, bfont_bgcolor, (bfont_32bit ? (sizeof (uint32)) : (sizeof (uint16))) << 3, opaque, str);
}

uint8 *bfont_find_icon(uint8 icon) {
    if(icon > BFONT_ICON_EMBROIDERY)
        return NULL;

    int icon_offset = BFONT_VMU_DREAMCAST_SPECIFIC +
        (icon * BFONT_ICON_DIMEN * BFONT_ICON_DIMEN/8);
    uint8 *fa = syscall_font_address();
    return fa + icon_offset;
}
