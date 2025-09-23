/* KallistiOS ##version##

   arch/null/include/arch/init_flags.h
   Copyright (C) 2001 Megan Potter
   Copyright (C) 2023 Lawrence Sebald
   Copyright (C) 2023 Falco Girgis

*/

/** \file    arch/init_flags.h
    \brief   Dreamcast-specific initialization-related flags and macros.
    \ingroup init_flags

    This file provides initialization-related flags that are specific to the
    Dreamcast architecture.

    \sa    kos/init.h
    \sa    kos/init_base.h

    \author Lawrence Sebald
    \author Megan Potter
    \author Falco Girgis
*/

#ifndef __ARCH_INIT_FLAGS_H
#define __ARCH_INIT_FLAGS_H

#include <kos/cdefs.h>
#include <kos/init_base.h>
__BEGIN_DECLS

/** \brief   Dreamcast-specific KOS_INIT Exports
    \ingroup init_flags

    This macro contains a list of all of the possible DC-specific
    exported functions based on their associated initialization flags.

    \note
    This is not typically used directly and is instead included within
    the top-level architecture-independent KOS_INIT_FLAGS() macro.

    \param flags    Parts of KOS to initialize.

    \sa KOS_INIT_FLAGS()
*/
#define KOS_INIT_FLAGS_ARCH(flags)  (void);


/** \defgroup kos_init_flags_dc Dreamcast-Specific Flags
    \brief    Dreamcast-specific initialization flags.
    \ingroup  init_flags

    These are the Dreamcast-specific flags that can be specified with
    KOS_INIT_FLAGS.

    \see    kos_initflags
    @{
*/

/** \brief Default init flags for the Dreamcast. */
#define INIT_DEFAULT_ARCH   (0)

/** @} */

__END_DECLS

#endif /* !__ARCH_INIT_FLAGS_H */
