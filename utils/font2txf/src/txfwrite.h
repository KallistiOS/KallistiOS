/* font2txf
 *
 * This code was contributed to KallistiOS (KOS) by Mickaël Cardoso (SiZiOUS).
 * It was originally made by Chris Laurel and the Celestia project team, for
 * producing the ttf2txf utility. The TXF format was created by Mark J. Kilgard.
 *
 * This code is licensed under GNU GPL 2, check LICENSE for details.
 */

#ifndef __TXFWRITE_H__
#define __TXFWRITE_H__

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>
#include <vector>

#include "global.h"
#include "utils.h"


struct TexGlyphInfo
{
    unsigned short c;       /* Potentially support 16-bit glyphs. */
    unsigned char width;
    unsigned char height;
    signed char xoffset;
    signed char yoffset;
    signed char advance;
    char dummy;           /* Space holder for alignment reasons. */
    short x;
    short y;
};


struct TexFontWriter
{
    enum eFormat
    {
        TXF_FORMAT_BYTE   = 0,
        TXF_FORMAT_BITMAP = 1
    };

    TexFontWriter() : tgi(0) {}
    ~TexFontWriter();

#if _DEBUG
    /* Dump the txf content to console (for debugging purpose). */
    void dump_to_console( bool crop = false);
#endif

    void set_glyph_count( int );
    void write( const char* filename );

    void display_info();

    int format;
    int tex_width;
    int tex_height;
    int max_ascent;
    int max_descent;
    int num_glyphs;

    //int min_glyph;
    //int range;

    unsigned char* tex_image;
    TexGlyphInfo* tgi;
};


#endif /* __TXFWRITE_H__ */
