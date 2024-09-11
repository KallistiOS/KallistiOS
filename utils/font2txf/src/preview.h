/* font2txf
 *
 * This code was contributed to KallistiOS (KOS) by Mickaël Cardoso (SiZiOUS).
 * It was originally made by Chris Laurel and the Celestia project team, for
 * producing the ttf2txf utility. The TXF format was created by Mark J. Kilgard.
 *
 * This code is licensed under GNU GPL 2, check LICENSE for details.
 */

#ifndef __PREVIEW_H__
#define __PREVIEW_H__

#ifdef DISPLAY

#include <ft2build.h>
#include FT_FREETYPE_H
#include <GL/glut.h>

#include "global.h"

/* Display converted, output txf file */
void do_preview_txf( int argc, char* argv[] );

#endif

#endif /* __PREVIEW_H__ */
