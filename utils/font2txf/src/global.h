/* font2txf
 *
 * This code was contributed to KallistiOS (KOS) by Mickaël Cardoso (SiZiOUS).
 * It was originally made by Chris Laurel and the Celestia project team, for
 * producing the ttf2txf utility. The TXF format was created by Mark J. Kilgard.
 *
 * This code is licensed under GNU GPL 2, check LICENSE for details.
 */

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <ft2build.h>
#include FT_FREETYPE_H
#include <vector>
#include "utils.h"

#define PROGRAM_NAME "font2txf"

#ifndef PROGRAM_VERSION
#define PROGRAM_VERSION "(undefined)"
#endif

#define DEFAULT_FONT_GAP 1
#define DEFAULT_FONT_SIZE 20
#define DEFAULT_FONT_HEIGHT 256
#define DEFAULT_FONT_WIDTH 256

/* Default Charset Codes */
#define DEFAULT_CHARCODES_POS0_SPC " "
#define DEFAULT_CHARCODES_POS1_AZU "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define DEFAULT_CHARCODES_POS2_NUM "1234567890"
#define DEFAULT_CHARCODES_POS3_AZL "abcdefghijklmnopqrstuvwxyz"
#define DEFAULT_CHARCODES_POS4_SYM "?.;,!*:\"/+-|'@#$%^&<>()[]{}_"

/* Default Charset Codes for the help screen */
#define DEFAULT_CHARCODES_POS1_AZU_SHORT "(A..Z)"
#define DEFAULT_CHARCODES_POS3_AZL_SHORT "(a..z)"
#define DEFAULT_CHARCODES "(space)" DEFAULT_CHARCODES_POS1_AZU_SHORT DEFAULT_CHARCODES_POS2_NUM DEFAULT_CHARCODES_POS3_AZL_SHORT DEFAULT_CHARCODES_POS4_SYM

/* Default characters to include in the TXF if nothing specified */
extern char g_default_char_codes[];

/* Characters to include in the TXF */
extern std::vector<wchar_t> g_char_codes;

/* Log level switch, basically: 0 = quiet, 1 = normal, 2 = verbose */
extern LogLevel g_log_level;

/* TXF data */
extern FT_Bitmap g_txf;

/* Console */
extern Console g_console;

/* Initialize some global variables and stuff. */
bool initialize( int argc, char* argv[] );

/* Log functions (binded to Console) */

/* Message: Log level (only if verbose) */
#define LOG( ... ) \
    if ( g_log_level != LogLevel::Quiet ) { \
        g_console.log( __VA_ARGS__ ); \
    }

/* Message: Warning Level */
#define WARN( ... ) \
    g_console.warn ( __VA_ARGS__ );

/* Message: Error level */
#define ERR( ... ) \
    g_console.error( __VA_ARGS__ );

/* Message: Fatal level */
#define FATAL( ... ) \
    g_console.fatal( __VA_ARGS__ );

/* Message: Debug level (only if _DEBUG is enabled) */
#define DEBUG( ... ) \
    g_console.debug( __VA_ARGS__ );

#endif /* __GLOBAL_H__ */
