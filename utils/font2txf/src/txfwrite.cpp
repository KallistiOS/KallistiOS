/* font2txf
 *
 * This code was contributed to KallistiOS (KOS) by Mickaël Cardoso (SiZiOUS).
 * It was originally made by Chris Laurel and the Celestia project team, for
 * producing the ttf2txf utility. The TXF format was created by Mark J. Kilgard.
 *
 * This code is licensed under GNU GPL 2, check LICENSE for details.
 */

#include "txfwrite.h"


TexFontWriter::~TexFontWriter()
{
    delete[] tgi;
}


void TexFontWriter::set_glyph_count( int n )
{
    num_glyphs = n;
    delete[] tgi;
    tgi = new TexGlyphInfo[ n ];
}


void TexFontWriter::write( const char* filename )
{
    FILE* fp;
    int endianness;

    assert( sizeof(int) == 4 );  /* Ensure external file format size. */

    fp = fopen( filename, "wb" );
    if( ! fp )
    {
        FATAL( "failed to open: \"", filename, "\"" );
        return;
    }

    endianness = 0x12345678;

    fwrite( "\377txf", 1, 4, fp );
    fwrite( &endianness,  sizeof(int), 1, fp );
    fwrite( &format,      sizeof(int), 1, fp );
    fwrite( &tex_width,   sizeof(int), 1, fp );
    fwrite( &tex_height,  sizeof(int), 1, fp );
    fwrite( &max_ascent,  sizeof(int), 1, fp );
    fwrite( &max_descent, sizeof(int), 1, fp );
    fwrite( &num_glyphs,  sizeof(int), 1, fp );
    
    for( int i = 0; i < num_glyphs; ++i )
    {
        tgi[ i ].dummy = 0;
        fwrite( &tgi[ i ], sizeof(TexGlyphInfo), 1, fp );
    }

    if( format == TXF_FORMAT_BITMAP )
    {
        FATAL( "TXF_FORMAT_BITMAP not handled\n" );
    }
    else
    {
#if 1
        unsigned char* row = tex_image + (tex_width * (tex_height - 1));
        for( int y = 0; y < tex_height; ++y )
        {
            fwrite( row, tex_width, 1, fp );
            row -= tex_width;
        }
#else
        fwrite( tex_image, tex_width * tex_height, 1, fp );
#endif
    }

    fclose( fp );
}


void TexFontWriter::display_info()
{
    std::string _format = "";
    switch (format) 
    {
        case TexFontWriter::eFormat::TXF_FORMAT_BITMAP:
            _format = "TXF_FORMAT_BITMAP";
            break;
        case TexFontWriter::eFormat::TXF_FORMAT_BYTE:
            _format = "TXF_FORMAT_BYTE";
            break;
        default:
            _format = "TXF_FORMAT_UNKNOWN";
            break;
    }

    switch( g_log_level )
    {
        case LogLevel::Verbose:
            std::cout << "TexFont [\n";
            std::cout << "  format:      " << _format << "\n";
            std::cout << "  tex_width:   " << tex_width << "\n";
            std::cout << "  tex_height:  " << tex_height << "\n";
            std::cout << "  max_ascent:  " << max_ascent << "\n";
            std::cout << "  max_descent: " << max_descent << "\n";
            std::cout << "  num_glyphs:  " << num_glyphs << "\n";
            std::cout << "]" << std::endl;
            break;

        case LogLevel::Standard:
            LOG( "writing ", num_glyphs, " glyphs in txf (width=", tex_width, ", height=", tex_height, ")" );
            break;

        case LogLevel::Quiet:
        default:
            break;
    }
}

#ifdef _DEBUG

void TexFontWriter::dump_to_console( bool crop )
{
    int x, y, w, h, pitch;    
    unsigned char* buf = tex_image;

    pitch = w = tex_width;
    h = tex_height;

    DEBUG( "txf dump:  pitch=", pitch, ", w=", w, ", h=", h, ", crop=", bool_to_str( crop ) );

    /* Fit on 80 column terminal. */
    if( crop && w > 39 )
        w = 39;

    for( y = 0; y < h; y++ )
    {
        for( x = 0; x < w; x++ )
        {
            int chr = (int) *(buf + x);
            printf( "%02x", chr );
        }
        std::cout << "\n";
        buf += pitch;
    }
    std::cout << std::endl;
}

#endif
