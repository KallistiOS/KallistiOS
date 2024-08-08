/* KallistiOS ##version##

   include/kos/init.h
   Copyright (C) 2001 Megan Potter
   Copyright (C) 2023 Lawrence Sebald
   Copyright (C) 2023 Paul Cercueil
   Copyright (C) 2023 Falco Girgis

*/

/** \file    kos/init.h
    \brief   Initialization-related flags and macros.
    \ingroup init_flags

    This file provides initialization-related flags and macros that can be used
    to set up various subsystems of KOS on startup. Only flags that are
    architecture-independent are specified here, however this file also includes
    the architecture-specific file to bring in those flags as well.

    \sa     arch/init_flags.h
    \sa     kos/init_base.h

    \author Megan Potter
    \author Lawrence Sebald
    \author Paul Cercueil
    \author Falco Girgis
*/

#ifndef __KOS_INIT_H
#define __KOS_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <arch/init_flags.h>
#include <kos/init_base.h>
#include <stdint.h>

/** \defgroup init_flags Initialization
    \brief               KOS Driver Subsystem and Component Initialization Flags
    \ingroup             system
*/

/** \cond */
#ifdef __cplusplus
#define __kos_cplusplus 1
#else
#define __kos_cplusplus 0
#endif

#define __KOS_INIT_FLAGS_0(flags) \
    const uint32_t __kos_init_flags = (flags); \
    KOS_INIT_FLAG(flags, INIT_NET, arch_init_net); \
    KOS_INIT_FLAG(flags, INIT_NET, net_shutdown); \
    KOS_INIT_FLAG(flags, INIT_NET, bba_la_init); \
    KOS_INIT_FLAG(flags, INIT_NET, bba_la_shutdown); \
    KOS_INIT_FLAG(flags, INIT_FS_ROMDISK, fs_romdisk_init); \
    KOS_INIT_FLAG(flags, INIT_FS_ROMDISK, fs_romdisk_shutdown); \
    KOS_INIT_FLAG(flags, INIT_EXPORT, export_init); \
    KOS_INIT_FLAG_NONE(flags, INIT_NO_SHUTDOWN, kos_shutdown); \
    KOS_INIT_FLAGS_ARCH(flags)

#define __KOS_INIT_FLAGS_1(flags) \
    extern "C" { \
        __KOS_INIT_FLAGS_0(flags); \
    }

#define __KOS_INIT_FLAGS(flags, cp) \
    __KOS_INIT_FLAGS_##cp(flags)

#define _KOS_INIT_FLAGS(flags, cp) \
    __KOS_INIT_FLAGS(flags, cp)

extern const uint32_t __kos_init_flags;
/** \endcond */

/** \brief   Exports and initializes the given KOS subsystems.
    \ingroup init_flags

    KOS_INIT_FLAGS() provides a mechanism through which various components
    of KOS can be enabled and initialized depending on whether their flag
    has been included within the list.

    \note
    When no KOS_INIT_FLAGS() have been explicitly provided, the default
    flags used by KOS are equivalent to KOS_INIT_FLAGS(INIT_DEFAULT).

    \param  flags           Parts of KOS to init.
 */
#define KOS_INIT_FLAGS(flags) \
    _KOS_INIT_FLAGS(flags, __kos_cplusplus)

/** \brief  Deprecated and not useful anymore. */
#define KOS_INIT_ROMDISK(rd) \
    const void *__kos_romdisk = (rd); \
    extern void fs_romdisk_mount_builtin_legacy(void); \
    void (*fs_romdisk_mount_builtin_legacy_weak)(void) = fs_romdisk_mount_builtin_legacy


/** \brief  Built-in romdisk. Do not modify this directly! */
extern const void * __kos_romdisk;

/** \brief  State that you don't want a romdisk. */
#define KOS_INIT_ROMDISK_NONE   NULL

/** \brief   Register a single function to be called very early in the boot
             process, before the BSS section is cleared.
    \ingroup init_flags

    \param  func            The function to register. The prototype should be
                            void func(void)
*/
#define KOS_INIT_EARLY(func) void (*__kos_init_early_fn)(void) = (func)

/** \defgroup kos_initflags     Generic Flags
    \brief                      Generic flags for use with KOS_INIT_FLAGS()
    \ingroup  init_flags

    These are the architecture-independent flags that can be specified with
    KOS_INIT_FLAGS.

    \see    dreamcast_initflags
    @{
*/

/** Init flags to include all virtual filesystems (default). */
#define INIT_FS_ALL     (INIT_FS_ROMDISK | INIT_FS_NULL | INIT_FS_PTY | \
                         INIT_FS_RAMDISK | INIT_FS_RND  | INIT_FS_DEV)

/** Default init flags (IRQs on, preemption enabled, romdisks). */
#define INIT_DEFAULT    (INIT_IRQ     | INIT_THD_PREEMPT | INIT_FS_ALL | \
                         INIT_LIBRARY | INIT_DEFAULT_ARCH)

#define INIT_NONE        0x00000000  /**< Don't init optional things */
#define INIT_IRQ         0x00000001  /**< Enable IRQs at startup */
#define INIT_THD_PREEMPT 0x00000002  /**< \deprecated Already default mode */
#define INIT_NET         0x00000004  /**< Enable built-in networking */
#define INIT_MALLOCSTATS 0x00000008  /**< Enable malloc statistics */
#define INIT_QUIET       0x00000010  /**< Disable dbgio */
#define INIT_EXPORT      0x00000020  /**< Export kernel symbols */
#define INIT_LIBRARY     0x00000040  /**< Enable support for dynamic libraries. */
#define INIT_FS_ROMDISK  0x00000080  /**< Enable support for romdisks */
#define INIT_FS_NULL     0x00000100  /**< Enable support for /dev/null VFS */
#define INIT_FS_PTY      0x00000200  /**< Enable support for PTY VFS */
#define INIT_FS_RAMDISK  0x00000400  /**< Enable support for ramdisk VFS */
#define INIT_FS_RND      0x00000800  /**< Enable support for /dev/urandom VFS */
#define INIT_FS_DEV      0x00001000  /**< Enable support for /dev/ VFS */
#define INIT_NO_SHUTDOWN 0x00002000  /**< Disable hardware shutdown */
/** @} */

__END_DECLS

#ifdef __cplusplus
};
#endif

#endif /* !__KOS_INIT_H */
