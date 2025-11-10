/* KallistiOS ##version##

   sys/utsname.h
   Copyright (C) 2018 Lawrence Sebald

*/

/** \file    utsname.h
    \brief   Definitions for the uname() function.
    \ingroup system_posix

    This file contains the definitions needed for using the uname() function,
    as directed by the POSIX 2008 standard (aka The Open Group Base
    Specifications Issue 7).

    \author Lawrence Sebald
*/

#ifndef __SYS_UTSNAME_H
#define __SYS_UTSNAME_H

#include <sys/cdefs.h>

__BEGIN_DECLS

/** \addtogroup system_posix
    @{
*/

#define _UTSNAME_LENGTH 64

/** \brief  Kernel name/information structure.

    This structure contains information about the kernel and is used by the
    uname() function for returning that information to a program.

    \headerfile sys/utsname.h
 */
struct utsname {
    char sysname[_UTSNAME_LENGTH];  /**< \brief OS Name ("KallistiOS"). */
    char nodename[_UTSNAME_LENGTH]; /**< \brief Name on network, if any. */
    char release[_UTSNAME_LENGTH];  /**< \brief Kernel release ("2.1.0"). */
    char version[_UTSNAME_LENGTH];  /**< \brief Kernel version string. */
    char machine[_UTSNAME_LENGTH];  /**< \brief Hardware identifier. */
};

/** \brief  Retrieve version and other similar information about the kernel.

    This function retrieves information about the current version of the kernel
    that is running, storing it in the provided buffer.

    \param  n           The buffer to store version information in.
    \return             0 on success, -1 on error (setting errno appropriately).
*/
int uname(struct utsname *n);

/** @} */

__END_DECLS

#endif /* !__SYS_UTSNAME_H */
