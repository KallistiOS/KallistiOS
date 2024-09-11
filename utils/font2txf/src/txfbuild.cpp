/* font2txf
 *
 * This code was contributed to KallistiOS (KOS) by Mickaël Cardoso (SiZiOUS).
 * It was originally made by Chris Laurel and the Celestia project team, for
 * producing the ttf2txf utility. The TXF format was created by Mark J. Kilgard.
 *
 * This code is licensed under GNU GPL 2, check LICENSE for details.
 */

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

#include "txfbuild.h"

#define FAILED_BUILD_TXF 0

#define FT_PIXELS(x)  (x >> 6)


void dump_char_maps( FT_Face face )
{
    FT_CharMap charmap;
    int n;

    printf( "  CharMaps %d [\n", face->num_charmaps );
    for( n = 0; n < face->num_charmaps; n++ )
    {
        long enc;
        charmap = face->charmaps[ n ];
        enc = charmap->encoding;
        printf( "    %lx (%c%c%c%c)\n", enc,
                 (char) ((enc >> 24) & 0xff),
                 (char) ((enc >> 16) & 0xff),
                 (char) ((enc >> 8) & 0xff),
                 (char) (enc & 0xff) );
    }
    printf( "  ]\n" );
}


void blit_glyph_to_bitmap( FT_Bitmap* src, FT_Bitmap* dst, int x, int y )
{
    unsigned char* s;
    unsigned char* d;
    unsigned char* dend;
    int r;

    s = src->buffer;
    d = dst->buffer + (y * dst->pitch) + x;
    dend = dst->buffer + (dst->rows * dst->pitch);

    r = src->rows;
    while( r && (d < dend) )
    {
        memcpy( d, s, src->width );
        s += src->pitch;
        d += dst->pitch;
        r--;
    }
}


static FT_Error render_glyph( FT_Bitmap* img, FT_GlyphSlot glyph,
                              int x_offset, int y_offset, bool antialias )
{
    /* first, render the glyph into an intermediate buffer */
    if( glyph->format != ft_glyph_format_bitmap )
    {
        FT_Error error = FT_Render_Glyph( glyph,
                antialias ? ft_render_mode_normal : ft_render_mode_mono );
        if( error )
            return error;
    }
 
#ifdef _DEBUG
    printf( "  KR offset %dx%d\n", x_offset, y_offset );
    printf( "  KR left/top %d %d\n", glyph->bitmap_left, glyph->bitmap_top );
    printf( "  KR metrics %ldx%ld %ldx%ld\n", 
            FT_PIXELS(glyph->metrics.width),
            FT_PIXELS(glyph->metrics.height),
            FT_PIXELS(glyph->metrics.horiBearingX),
            FT_PIXELS(glyph->metrics.horiBearingY) );
#endif

    /* Then, blit the image to the target surface */
    blit_glyph_to_bitmap( &glyph->bitmap, img,
                          x_offset + glyph->bitmap_left,
                          y_offset - glyph->bitmap_top );

    return 0;
}


/* Build the TXF (textured font).
 * Returns number of glyphs added or zero if fails.
 * If glyphs < 0, it means conversion happened with errors/warnings. */
int build_txf( TexFontWriter& fontw,
               const char* file,
               const std::vector<wchar_t>& codes,
               FT_Bitmap* img,
               int psize,
               int gap,
               bool asBitmap )
{
    FT_Library library;
    FT_Face face;
    FT_Error error;
    FT_GlyphSlot glyph;
    FT_Size size;
    FT_F26Dot6 start_x, step_y, x, y;
    int count = FAILED_BUILD_TXF, fail = 0;
    bool init_ft_lib = true,
        init_ft_face = true,
        init_ft_pixel_sizes = true,
        is_conversion_completely_successful = false;


    error = FT_Init_FreeType( &library );
    if( error )
    {
        init_ft_lib = false;
        ERR( "unable to initialize FreeType library" );
    }


    if ( init_ft_lib )
    {
        error = FT_New_Face( library, file, 0, &face );
        if( error )
        {
            init_ft_face = false;
            ERR( "unable to initialize new face" );
        }


        if( init_ft_face )
        {
            error = FT_Set_Pixel_Sizes( face, psize, psize );
            if( error )
            {
                init_ft_pixel_sizes = false;
                ERR( "unable to set pixel sizes" );
            }


            if( init_ft_pixel_sizes )
            {

                switch( g_log_level)
                {
                    case LogLevel::Verbose:
                        printf( "FT_Face [\n" );
                        printf( "  family_name: \"%s\"\n", face->family_name );
                        printf( "  style_name:  \"%s\"\n",  face->style_name );
                        printf( "  num_glyphs:  %ld\n", face->num_glyphs );
                        dump_char_maps( face );
                        printf( "]\n" );
                        break;

                    case LogLevel::Standard:
                        LOG( "using font: ", face->family_name, " (", face->style_name, ")" );
                        break;

                    case LogLevel::Quiet:
                    default:
                        break;
                }

                LOG( "starting txf generation" );
                is_conversion_completely_successful = true;

                glyph = face->glyph;
                size  = face->size;

                fontw.set_glyph_count( face->num_glyphs );
                // fontw.max_ascent  = size->metrics.y_ppem;
                // fontw.max_ascent  = FT_PIXELS(face->ascender);
                // fontw.max_descent = -FT_PIXELS(face->descender);
                fontw.max_ascent  = FT_PIXELS( (int) (face->ascender * (float) psize / 30.0f) );
                fontw.max_descent = -FT_PIXELS( (int) (face->descender * (float) psize / 30.0f) );

                /* Clear bitmap */
                memset( img->buffer, 0, img->rows * img->pitch );

                step_y = size->metrics.y_ppem + gap;
                start_x = gap;
                x = start_x;
                y = step_y;

                for (unsigned int i = 0; i < codes.size(); i++)
                {
                    wchar_t current_charcode = codes[i];

                    int glyph_index = FT_Get_Char_Index( face, current_charcode );
                    if( glyph_index == 0 )
                    {
                        WARN( "character code ", int_to_hex( current_charcode ), " is undefined" );
                        is_conversion_completely_successful = false;
                        continue;
                    }

                    error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );
                    if( ! error )
                    {
                        unsigned int nextX = x + FT_PIXELS(glyph->metrics.horiAdvance) + gap;

                        if( nextX > img->width )
                        {
                            x  = start_x;
                            y += step_y;

                            if( (unsigned int) y >= img->rows )
                            {
                                WARN( "texture too small for ", psize, "pt \"", file, "\"" );
                                is_conversion_completely_successful = false;
                                break;
                            }

                            nextX = x + FT_PIXELS(glyph->metrics.horiAdvance) + gap;
                        }

                        render_glyph( img, glyph, x, y, ! asBitmap );

                        TexGlyphInfo& tgi = fontw.tgi[ count ];
                        count++;

                        tgi.c       = current_charcode;
                        tgi.width   = FT_PIXELS(glyph->metrics.width);
                        tgi.height  = FT_PIXELS(glyph->metrics.height);
                        tgi.xoffset = FT_PIXELS(glyph->metrics.horiBearingX);
                        tgi.yoffset = FT_PIXELS(glyph->metrics.horiBearingY) - tgi.height;
                        tgi.advance = FT_PIXELS(glyph->metrics.horiAdvance);
                        tgi.x       = x + tgi.xoffset;
                        tgi.y       = fontw.tex_height - y + tgi.yoffset;

#ifdef _DEBUG
                        printf( "char: \"%c\"  code: %04x  size=%dx%d\n", tgi.c, tgi.c, tgi.width, tgi.height );
#endif

                        x = nextX;
                    }
                    else
                    {
                        WARN( "unable to load glyph for ", int_to_hex( current_charcode ) );                        
                        fail++;
                    }
                } // for

                if( ! count )
                {
                    is_conversion_completely_successful = false;
                    FATAL( "there is no glyphs in this font" );                    
                }
                else if( fail )
                {
                    is_conversion_completely_successful = false;
                    WARN( "failed to load ", fail, " glyphs" );
                }
            } // init_ft_pixel_sizes

            DEBUG( "destroying font face" );
            FT_Done_Face( face );
        } // init_ft_face

        DEBUG( "destroying font library" );
        FT_Done_FreeType( library );
    } // init_ft_lib

    if( ! is_conversion_completely_successful )
    {
        // reverse the sign to notify the caller that we indeed converted but errors/warnings happened...
        count = count * -1;
    }

    return count;
}
