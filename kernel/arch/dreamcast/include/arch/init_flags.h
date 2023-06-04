/* KallistiOS ##version##

   arch/dreamcast/include/arch/init_flags.h
   Copyright (C) 2001 Megan Potter
   Copyright (C) 2023 Lawrence Sebald

*/

/** \file   arch/init_flags.h
    \brief  Dreamcast-specific initialization-related flags and macros.

    This file provides initialization-related flags that are specific to the
    Dreamcast architecture.

    \author Lawrence Sebald
    \author Megan Potter
    \see    kos/init.h
*/

#ifndef __ARCH_INIT_FLAGS_H
#define __ARCH_INIT_FLAGS_H

#include <kos/cdefs.h>
__BEGIN_DECLS

/** \defgroup dreamcast_initflags   Dreamcast-specific initialization flags.

    These are the Dreamcast-specific flags that can be specified with
    KOS_INIT_FLAGS.

    \see    kos_initflags
    @{
*/
#define INIT_OCRAM          0x10000 /**< \brief Use half of the dcache as RAM */
#define INIT_NO_DCLOAD      0x20000 /**< \brief Disable dcload */

/** @} */

__END_DECLS

#endif /* !__ARCH_INIT_FLAGS_H */
