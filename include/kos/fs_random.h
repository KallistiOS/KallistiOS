/* KallistiOS ##version##

   kos/fs_random.h
   Copyright (C) 2023 Luke Benstead

*/

/** \file    kos/fs_random.h
    \brief   Driver for /dev/random and /dev/urandom.
    \ingroup vfs_rnd

    This filesystem driver provides implementations of /dev/random
    and /dev/urandom for portability. It seeds randomness from
    uninitialized memory, and the clock. Obviously we
    are limited in how we can provide sufficient entropy on an
    embedded platform like the Dreamcast so the randomness from
    this driver will not win any awards but it should be sufficiently
    good for most purposes.

    /dev/random is an alias to /dev/urandom for now.

    \author Luke Benstead
*/

#ifndef __DC_FS_RANDOM_H
#define __DC_FS_RANDOM_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/fs.h>

/** \defgroup vfs_rnd   Random
    \brief              VFS driver for /dev/random and /dev/urandom
    \ingroup            vfs

    @{
*/

/* \cond */
/* Initialization */
void fs_rnd_init(void);
void fs_rnd_shutdown(void);
/* \endcond */

/** @} */

__END_DECLS

#endif  /* __DC_FS_RANDOM_H */

