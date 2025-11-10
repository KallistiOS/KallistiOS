/* KallistiOS ##version##

   kos/banner.h
   Copyright (C) 2013 Lawrence Sebald
   Copyright (C) 2025 Eric Fradella
*/

#ifndef __KOS_BANNER_H
#define __KOS_BANNER_H

#include <kos/cdefs.h>
__BEGIN_DECLS

/** \file       kos/banner.h
    \defgroup attribution  Attribution
    \brief                 KOS banner, license, and authors

    This API can be used to query for and display information
    on KOS, its license, and its authors at runtime.

    \remark
    The authors list can be used for credits screens in games
    and applications to acknowledge KOS and its contributors. :)
*/

/** \brief   Retrieve the banner printed at program initialization.
    \ingroup attribution

    This function retrieves the banner string that is printed at initialization
    time by the kernel. This contains the version of KOS in use and basic
    information about the environment in which it was compiled.

    \return                 A pointer to the banner string.
*/
const char * __pure2 kos_get_banner(void);

/** \brief   Retrieve the license information for the compiled copy of KOS.
    \ingroup attribution

    This function retrieves a string containing the license terms that the
    version of KOS in use is distributed under. This can be used to easily add
    information to your program to be displayed at runtime.

    \return                 A pointer to the license terms.
*/
const char * __pure2 kos_get_license(void);

/** \brief   Retrieve a list of authors and the dates of their contributions.
    \ingroup attribution

    This function retrieves the copyright information for the version of KOS in
    use. This function can be used to add such information to the credits of
    programs using KOS to give the appropriate credit to those that have worked
    on KOS.

    \remark
    Remember, you do need to give credit where credit is due, and this is an
    easy way to do so. ;-)

    \return                 A pointer to the authors' copyright information.
*/
const char *__pure2 kos_get_authors(void);

__END_DECLS

#endif /* !__KOS_BANNER_H */
