/* KallistiOS ##version##

   sys/uio.h
   Copyright (C) 2017 Lawrence Sebald

*/

/** \file    sys/uio.h
    \brief   Header for vector I/O.
    \ingroup vfs_posix

    This file contains definitions for vector I/O operations, as specified by
    the POSIX 2008 specification. Full vector-based I/O is not supported for
    file operations, but the stuff in here is still useful elsewhere.

    \author Lawrence Sebald
*/

#ifndef __SYS_UIO_H
#define __SYS_UIO_H

#include <kos/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

/** \addtogroup vfs_posix
    @{
*/

/** \brief  I/O vector structure
    \headerfile sys/uio.h
*/
struct iovec {
    /** \brief  Base address of memory for I/O. */
    void * iov_base;
    /** \brief  Size of memory pointed to by iov_base. */
    size_t iov_len;
};

/** \brief  Old alias for the maximum length of an iovec. */
#define UIO_MAXIOV IOV_MAX

/** @} */

__END_DECLS

#endif /* __SYS_UIO_H */
