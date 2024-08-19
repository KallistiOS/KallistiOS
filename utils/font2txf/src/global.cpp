/* font2txf
 *
 * This code was contributed to KallistiOS (KOS) by Mickaël Cardoso (SiZiOUS).
 * It was originally made by Chris Laurel and the Celestia project team, for
 * producing the ttf2txf utility. The TXF format was created by Mark J. Kilgard.
 *
 * This code is licensed under GNU GPL 2, check LICENSE for details.
 */

#include <assert.h>
#include "global.h"


/* Check header file for the description of the following global variables */
char g_default_char_codes[] = DEFAULT_CHARCODES_POS0_SPC DEFAULT_CHARCODES_POS1_AZU DEFAULT_CHARCODES_POS2_NUM DEFAULT_CHARCODES_POS3_AZL DEFAULT_CHARCODES_POS4_SYM;
std::vector<wchar_t> g_char_codes;
LogLevel g_log_level = LogLevel::Standard;
FT_Bitmap g_txf;
Console g_console;


/* Perform all clean-up when the program exits. */
void finalize()
{
    DEBUG( "finalize fired!" );

    if( g_txf.buffer )
    {        
        free( g_txf.buffer );
        g_txf.buffer = nullptr;
    }
}


/* Initialize some global variables and stuff. */
bool initialize( int argc, char* argv[] )
{
    assert( argc );
    program_name_initialize( argv[0] );

    if ( std::atexit( finalize ) )
    {
        FATAL( "initialize: atexit() registration failed!" );
        return false;
    }

    return true;
}

