/* font2txf
 *
 * This code was contributed to KallistiOS (KOS) by Mickaël Cardoso (SiZiOUS).
 * It was originally made by Chris Laurel and the Celestia project team, for
 * producing the ttf2txf utility. The TXF format was created by Mark J. Kilgard.
 *
 * This code is licensed under GNU GPL 2, check LICENSE for details.
 */

/*============================================================================
//
// $Id: ttf2txf.cpp,v 1.11 2001/10/20 23:46:37 karl Exp $
//
// Converts TrueType, Type 1 and OpenType fonts to TXF texture font format.
// Uses FreeType 2.x.
//
//==========================================================================*/

#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "txfbuild.h"
#include "charset.h"
#include "preview.h"


/* Display the header... */
void header()
{
    std::cout << PROGRAM_NAME << ", version " << PROGRAM_VERSION << " (built on " << __DATE__ << ")\n\n";
}


/* Display help/usage of this tool. */
void usage()
{
    header();

    std::cout << "Converts a TrueType/OpenType font file <fontfile.ttf/otf> to a texture mapped\n";
    std::cout << "font (TXF), the font format created by Mark J. Kilgard for the OpenGL Utility\n";
    std::cout << "Toolkit (GLUT).  This tool is an updated version of \"ttf2txf\" originally\n";
    std::cout << "written by Chris Laurel for the Celestia project.\n\n";    

    std::cout << "Usage: " << program_name_get() << " [options] <fontfile.ttf/otf>\n\n";

    std::cout << "Default charset to convert (see `-c` or `-f` options below): " << "\n";
    std::cout << "  " << DEFAULT_CHARCODES << "\n\n";

    std::cout << "Options:\n";
    std::cout << "  -w <width>         Texture width (default: " << DEFAULT_FONT_WIDTH << ")\n";
    std::cout << "  -e <height>        Texture height (default: " << DEFAULT_FONT_HEIGHT << "); also `-h` for compatibility\n";
#if 0 /* Disabled for now */
    std::cout << "  -b                 Create bitmap texture\n";
#endif
    std::cout << "  -c <string>        Override charset to convert; read from command-line\n";
    std::cout << "                     Cannot be mixed with `-f`\n";
    std::cout << "  -f <filename.txt>  Override charset to convert; read from a text file\n";
    std::cout << "                     Cannot be mixed with `-c`\n";
    std::cout << "  -g <gap>           Space between glyphs (default: " << DEFAULT_FONT_GAP << ")\n";
    std::cout << "  -s <size>          Font point size (default: " << DEFAULT_FONT_SIZE << ")\n";
    std::cout << "  -o <filename.txf>  Output file for textured font (default: <fontfile>.txf)\n";
    std::cout << "  -q                 Quiet; except error messages, cannot be mixed with `-v`\n";
    std::cout << "  -v                 Verbose; display more info, cannot be mixed with `-q`\n";
#ifdef DISPLAY
    std::cout << "  -p                 Preview; display the txf output at the end of the process\n";
#endif
	std::cout << "  -h                 Usage information (you're looking at it); if `-w` not set";

    std::cout << std::endl;
}


/* Entry point */
int main( int argc, char* argv[] )
{
    TexFontWriter fontw;
    int i,
        gap = DEFAULT_FONT_GAP,
        size = DEFAULT_FONT_SIZE,
        txf_encoded_glyphs = 0;
    bool asBitmap = false,
        txf_encoded_without_issues = false,
        c_switch = false,
        h_switch = false,
        q_switch = false,
        v_switch = false;
    char* infile = 0;
    char outfile[ FILENAME_MAX ];
    std::string codesfile;
    char* codes = g_default_char_codes;
#ifdef DISPLAY
    bool preview_txf = false;
#endif

    if( !initialize( argc, argv ) )
    {
        return EXIT_FAILURE;
    }

    if( argc < 2 )
    {       
        usage();       
        return EXIT_SUCCESS;
    }

    outfile[ 0 ] = '\0';

    fontw.format     = TexFontWriter::TXF_FORMAT_BYTE;
    fontw.tex_width  = DEFAULT_FONT_WIDTH;
    fontw.tex_height = DEFAULT_FONT_HEIGHT;

    /* Simple options parsing */
    for( i = 1; i < argc; i++ )
    {
        if( *argv[i] == '-' )
        {
            char* cp = ( argv[ i ] + 1 );
            
            if( *cp == 'w' )
            {
                /* Width */                
                i++;
                if( i >= argc )
                    break;
                fontw.tex_width = atoi( argv[ i ] );
            }
            else if( *cp == 'e' || *cp == 'h' )
            {
                /* Height ("e" but could be "h" for compatibility) */
                
                /* Handle compatibility */
                h_switch = true;
                
                i++;
                if( i >= argc )
                    break;
                fontw.tex_height = atoi( argv[ i ] ); 

                /* Reevaluate h_switch; if tex_height > 0, then finally it's "height" */
                h_switch = ( ! fontw.tex_height );
            }
            else if( *cp == 'c' )
            {
                /* Characters to convert (from command-line) */
                i++;
                if( i >= argc )
                    break;
                codes = argv[ i ];
                c_switch = true;
            }
#if 0
            else if( *cp == 'b' )
            {
                /* Bitmap texture */
                asBitmap = true;
            }
#endif
            else if( *cp == 'g' )
            {
                /* Spaces between glyphs (gap) */
                i++;
                if( i >= argc )
                    break;
                gap = atoi( argv[ i ] );
            }
            else if( *cp == 's' )
            {
                /* Font point size */
                i++;
                if( i >= argc )
                    break;
                size = atoi( argv[ i ] );
            }
            else if( *cp == 'o' )
            {
                /* Output txf filename */
                i++;
                if( i >= argc )
                    break;
                strcpy( outfile, argv[ i ] );
            }
            else if( *cp == 'q' )
            {
                /* Quiet mode */
                q_switch = true;
                g_log_level = LogLevel::Quiet;
            }
            else if( *cp == 'v' )
            {
                /* Verbose mode */
                v_switch = true;
                g_log_level = LogLevel::Verbose;
            }
            else if( *cp == 'f' )
            {
                i++;
                if (i >= argc)
                    break;
                codesfile = argv[ i ];
            }
#ifdef DISPLAY            
            else if( *cp == 'p' )
            {
                preview_txf = true;      
            }
#endif            
        }
        else
        {
            /* Input ttf/otf file */
            infile = argv[ i ];
        }
    }

    /* Display help info, if no input file is provided and "-h" is used "alone" (without "-w") */
    if( h_switch && ! infile )
    {
        usage();
        return EXIT_SUCCESS;
    }

    if ( v_switch || g_log_level != LogLevel::Quiet )
    {
        header();
    }

    /* Check if a input font has been passed */
    if( ! infile )
    {
        FATAL( "unspecified input font file" );        
        return EXIT_FAILURE;
    }

    /* Check if input file is provided */
    if( ! file_exists(infile) )
    {        
        FATAL( "input file not found" );
        return EXIT_FAILURE;
    }
	
    /* Options "-c" and "-f" can't be mixed */
    if( c_switch && ! codesfile.empty() )
	{
		ERR( "unable to use `-c` and `-f` options at the same time" );
        return EXIT_FAILURE;
	}

    /* Options "-q" and "-v" can't be mixed */
    if( q_switch && v_switch )
	{
		ERR( "unable to use `-q` and `-v` options at the same time" );
        return EXIT_FAILURE;
	}

    /* Set outfile to base infile and append ".txf" */
    if( outfile[ 0 ] == '\0' )
    {
        char* src = infile;
        char* dst = outfile;
        while( *src )
        {
            if( *src == '/' || *src == '\\' )
            {
                // Reset to strip path.
                dst = outfile;
            }
            else if( *src == '.' )
            {
                break;
            }
            else
            {
                *dst++ = *src;
            }
            ++src;
        }
        strcpy( dst, ".txf" );
    }

    g_txf.width  = fontw.tex_width;
    g_txf.rows   = fontw.tex_height;
    g_txf.pitch  = g_txf.width;
    g_txf.buffer = (unsigned char*) malloc( g_txf.pitch * g_txf.rows );

    // Populate the list of character codes
    if ( !codesfile.empty() )
    {
        // Populate from the text file (-f)
        if ( !load_charcodes_file( codesfile ) )
        {
            FATAL( "cannot load the charset from the specified text file" );
            return EXIT_FAILURE;
        }
    }
    else
    {
        // Populate from the command-line (-c) OR using default charset
        if( c_switch )
        {
            LOG( "setting up new charset: \"", codes, "\"" );
        }

        i = 0;
        while ( codes[i] != '\0' )
        {
            g_char_codes.insert( g_char_codes.end(), (wchar_t) codes[i] );
            i++;
        }
    }
    // At this point, the charset is assigned into g_char_codes

    txf_encoded_glyphs = build_txf( fontw, infile, g_char_codes, &g_txf, size, gap,
                                    asBitmap );
    txf_encoded_without_issues = ( txf_encoded_glyphs > 0 );
    
    fontw.num_glyphs = abs( txf_encoded_glyphs );

    if( ! fontw.num_glyphs )
	{
        FATAL( "failed building font" );
        return EXIT_FAILURE;
	}

    fontw.display_info();

    fontw.tex_image = g_txf.buffer;
    fontw.write( outfile );

#if _DEBUG && _DEBUG_FONT_DUMP_TO_CONSOLE
    fontw.dump_to_console();
#endif

    if( txf_encoded_without_issues )
    {
        LOG( "txf written successfully" );
    }
    else
    {
        WARN( "txf written with issues" );
    }

#ifdef DISPLAY
    if ( preview_txf )
    {
        LOG( "displaying txf preview... close the preview window to exit" );
        do_preview_txf( argc, argv );
    }
#endif

    return EXIT_SUCCESS;
}


/*EOF*/
