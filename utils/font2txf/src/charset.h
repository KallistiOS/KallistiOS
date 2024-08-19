/* font2txf
 *
 * This code was contributed to KallistiOS (KOS) by Mickaël Cardoso (SiZiOUS).
 * It was originally made by Chris Laurel and the Celestia project team, for
 * producing the ttf2txf utility. The TXF format was created by Mark J. Kilgard.
 *
 * This code is licensed under GNU GPL 2, check LICENSE for details.
 */

#ifndef __CHARSET_H__
#define __CHARSET_H__

#include <string>
#include "global.h"

/* Load charcodes file from command line */
bool load_charcodes_file(const std::string& filename);

#endif /* __CHARSET_H__ */
