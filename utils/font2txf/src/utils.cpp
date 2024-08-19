/* font2txf
 *
 * This code was contributed to KallistiOS (KOS) by Mickaël Cardoso (SiZiOUS).
 * It was originally made by Chris Laurel and the Celestia project team, for
 * producing the ttf2txf utility. The TXF format was created by Mark J. Kilgard.
 *
 * This code is licensed under GNU GPL 2, check LICENSE for details.
 */

#include <filesystem>
#include <libgen.h>

#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <fstream>

#include <memory>
#include <string>
#include <stdexcept>

#include <iostream>
#include <sstream>

#include "utils.h"

std::string g_program_name;

void program_name_initialize( char* argv0 )
{
    std::string program_filename( basename( argv0 ) );
    size_t dotchr = program_filename.find_last_of( "." );
    g_program_name = ( dotchr == std::string::npos ) ? program_filename : program_filename.substr( 0, dotchr );
}

std::string program_name_get()
{
    return g_program_name;
}

std::string bool_to_str( bool b )
{
    return ( b ? "true" : "false" );
}

std::string int_to_hex(int hex_val)
{
    if (hex_val == 0) {
        return "0x0";
    }
    else if (hex_val < 0) {
        hex_val = -hex_val;
    }

    std::stringstream ss;
    ss << "0x" << std::hex << hex_val;
    return ss.str();
}

bool file_exists( const std::string& name )
{
  struct stat buf;   
  return ( stat ( name.c_str(), &buf ) == 0 ); 
}
