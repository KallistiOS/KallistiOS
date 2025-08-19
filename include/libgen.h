/* KallistiOS ##version##

   libgen.h
   Copyright (C) 2020 Lawrence Sebald
*/

/** \file    libgen.h
    \brief   Definitions for pattern matching functions.
    \ingroup vfs_posix

    This file contains the definitions for the functions basename() and
    dirname() as specified by the POSIX standard. How the POSIX people came
    up with this filename for this stuff, I don't know.

    The GNU C library defines a slightly different version of these functions in
    <string.h> instead of here. Please note that these functions, unlike the
    GNU versions that are in <string.h> on Linux do modify their input values,
    as POSIX allows.

    \author Lawrence Sebald
*/

#ifndef __LIBGEN_H
#define __LIBGEN_H

#include <kos/cdefs.h>
__BEGIN_DECLS

/** \addtogroup vfs_posix
    @{
*/

/** \brief  Get the last component of a pathname.

    This function retrieves the basename from a given path. The basename of a
    path is the non-directory component thereof, minus any trailing '/'
    characters. This function does not attempt to perform any sort of path
    resolution.

    \note   This function may modify its input and the returned value may be a
            pointer to part of its input.

    \param  path            The path to extract the basename from.
    \return                 A pointer to the basename of the given path.
*/
char *basename(char *path);

/** \brief  Get the parent directory of a file pathname.

    This function retrieves the name of the parent directory of the file
    pathname specified. This function does not attempt to perform any sort of
    path resolution to check for the existence of the specified directory or to
    resolve any symbolic links.

    \note   This function may modify its input and the returned value may be a
            pointer to part of its input.

    \param  path            The path to extract the parent directory from.
    \return                 A pointer to the parent directory of the given path.
*/
char *dirname(char *path);

/** @} */

__END_DECLS

#endif /* !__LIBGEN_H */
