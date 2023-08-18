/* KallistiOS ##version##

   kos/limits.h
   (c)2000-2001 Megan Potter

*/

/** \file   kos/limits.h
    \brief  Limits.

    This file contains definitions of limits of various things.

    \author Megan Potter
*/

#ifndef __KOS_LIMITS_H
#define __KOS_LIMITS_H

#ifndef NAME_MAX
#define NAME_MAX    256     /**< \brief Max filename length */
#endif

/* MAX_FN_LEN defined for legacy code compatibility */
#ifndef MAX_FN_LEN
#define MAX_FN_LEN  NAME_MAX
#endif

#ifndef PATH_MAX
#define PATH_MAX    4096    /**< \brief Max path length */
#endif

#ifndef SYMLOOP_MAX
#define SYMLOOP_MAX 16      /**< \brief Max number of symlinks resolved */
#endif

#endif  /* __KOS_LIMITS_H */
