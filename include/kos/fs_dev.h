/* KallistiOS ##version##

   kos/fs_dev.h
   (c)2023 - Luke Benstead

*/

/** \file   kos/fs_dev.h
    \brief  Driver for /dev/random and /dev/urandom.

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

#ifndef __DC_FS_DEV_H
#define __DC_FS_DEV_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/fs.h>

/* \cond */
/* Initialization */
int fs_dev_init(void);
int fs_dev_shutdown(void);
/* \endcond */

__END_DECLS

#endif  /* __DC_FS_DEV_H */

