/* KallistiOS ##version##

   arch/dreamcast/include/arch/init_flags.h
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
#define KOS_INIT_FLAGS_ARCH(flags) \
    KOS_INIT_FLAG_NONE(flags, INIT_NO_DCLOAD, dcload_init); \
    KOS_INIT_FLAG_NONE(flags, INIT_NO_DCLOAD, fs_dcload_init_console); \
    KOS_INIT_FLAG_NONE(flags, INIT_NO_DCLOAD, fs_dcload_shutdown); \
    KOS_INIT_FLAG_NONE(flags, INIT_NO_DCLOAD, arch_init_net_dcload_ip); \
    KOS_INIT_FLAG(flags, INIT_NO_DCLOAD, arch_init_net_no_dcload); \
    KOS_INIT_FLAG(flags, INIT_CDROM, cdrom_init); \
    KOS_INIT_FLAG(flags, INIT_CDROM, cdrom_shutdown); \
    KOS_INIT_FLAG(flags, INIT_CDROM, fs_iso9660_init); \
    KOS_INIT_FLAG(flags, INIT_CDROM, fs_iso9660_shutdown); \
    KOS_INIT_FLAG(flags, INIT_CONTROLLER, cont_init); \
    KOS_INIT_FLAG(flags, INIT_CONTROLLER, cont_shutdown); \
    KOS_INIT_FLAG(flags, INIT_KEYBOARD, kbd_init); \
    KOS_INIT_FLAG(flags, INIT_KEYBOARD, kbd_shutdown); \
    KOS_INIT_FLAG(flags, INIT_MOUSE, mouse_init); \
    KOS_INIT_FLAG(flags, INIT_MOUSE, mouse_shutdown); \
    KOS_INIT_FLAG(flags, INIT_LIGHTGUN, lightgun_init); \
    KOS_INIT_FLAG(flags, INIT_LIGHTGUN, lightgun_shutdown); \
    KOS_INIT_FLAG(flags, INIT_VMU, vmu_init); \
    KOS_INIT_FLAG(flags, INIT_VMU, vmu_shutdown); \
    KOS_INIT_FLAG(flags, INIT_VMU, vmu_fs_init); \
    KOS_INIT_FLAG(flags, INIT_VMU, vmu_fs_shutdown); \
    KOS_INIT_FLAG(flags, INIT_PURUPURU, purupuru_init); \
    KOS_INIT_FLAG(flags, INIT_PURUPURU, purupuru_shutdown); \
    KOS_INIT_FLAG(flags, INIT_SIP, sip_init); \
    KOS_INIT_FLAG(flags, INIT_SIP, sip_shutdown); \
    KOS_INIT_FLAG(flags, INIT_DREAMEYE, dreameye_init); \
    KOS_INIT_FLAG(flags, INIT_DREAMEYE, dreameye_shutdown); \
    KOS_INIT_FLAG(flags, INIT_MAPLE_ALL, maple_wait_scan); \
    KOS_INIT_FLAG(flags, INIT_MAPLE_ALL, maple_init); \
    KOS_INIT_FLAG(flags, INIT_MAPLE_ALL, maple_shutdown)


/** \defgroup kos_init_flags_dc Dreamcast-Specific Flags
    \brief    Dreamcast-specific initialization flags.
    \ingroup  init_flags

    These are the Dreamcast-specific flags that can be specified with
    KOS_INIT_FLAGS.

    \see    kos_initflags
    @{
*/

/** \brief Default init flags for the Dreamcast. */
#define INIT_DEFAULT_ARCH   (INIT_MAPLE_ALL | INIT_CDROM)

/** \brief Enable all Maple peripheral drivers. */
#define INIT_MAPLE_ALL      (INIT_CONTROLLER | INIT_KEYBOARD | INIT_MOUSE    | \
                             INIT_LIGHTGUN   | INIT_VMU      | INIT_PURUPURU | \
                             INIT_SIP        | INIT_DREAMEYE)

#define INIT_CONTROLLER     0x00004000  /**< \brief Enable Controller maple driver */
#define INIT_KEYBOARD       0x00008000  /**< \brief Enable Keyboard maple driver */
#define INIT_MOUSE          0x00010000  /**< \brief Enable Mouse maple driver */
#define INIT_LIGHTGUN       0x00020000  /**< \brief Enable Lightgun maple driver */
#define INIT_VMU            0x00040000  /**< \brief Enable VMU maple driver */
#define INIT_PURUPURU       0x00080000  /**< \brief Enable Puru Puru maple driver */
#define INIT_SIP            0x00100000  /**< \brief Enable Sound input maple driver */
#define INIT_DREAMEYE       0x00200000  /**< \brief Enable DreamEye maple driver */

#define INIT_CDROM          0x00100000  /**< \brief Enable CD-ROM support */

#define INIT_OCRAM          0x10000000  /**< \brief Use half of the dcache as RAM */
#define INIT_NO_DCLOAD      0x20000000  /**< \brief Disable dcload */

/** @} */

__END_DECLS

#endif /* !__ARCH_INIT_FLAGS_H */
