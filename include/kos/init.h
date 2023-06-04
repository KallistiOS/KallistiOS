/* KallistiOS ##version##

   include/kos/init.h
   Copyright (C) 2001 Megan Potter
   Copyright (C) 2023 Lawrence Sebald

*/

/** \file   kos/init.h
    \brief  Initialization-related flags and macros.

    This file provides initialization-related flags and macros that can be used
    to set up various subsystems of KOS on startup. Only flags that are
    architecture-independent are specified here, however this file also includes
    the architecture-specific file to bring in those flags as well.

    \author Lawrence Sebald
    \author Megan Potter
    \see    arch/init_flags.h
*/

#ifndef __KOS_INIT_H
#define __KOS_INIT_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <arch/init_flags.h>

/** \brief  Use this macro to determine the level of initialization you'd like
            in your program by default.

    The defaults will be fine for most things, and will be used if you do not
    specify any init flags yourself.

    \param  flags           Parts of KOS to init.
    \see    kos_initflags
    \see    dreamcast_initflags
*/
#define KOS_INIT_FLAGS(flags)   uint32 __kos_init_flags = (flags)

/** \brief  The init flags. Do not modify this directly! */
extern uint32 __kos_init_flags;

/** \brief  Define a romdisk for your program, if you'd like one.
    \param  rd                  Pointer to the romdisk image in your code.
*/
#define KOS_INIT_ROMDISK(rd)    void * __kos_romdisk = (rd)

/** \brief  Built-in romdisk. Do not modify this directly! */
extern void * __kos_romdisk;

/** \brief  State that you don't want a romdisk. */
#define KOS_INIT_ROMDISK_NONE   NULL

/** \brief  Register a single function to be called very early in the boot
            process, before the BSS section is cleared.

    \param  func            The function to register. The prototype should be
                            void func(void)
*/
#define KOS_INIT_EARLY(func) void (*__kos_init_early_fn)(void) = (func)

/** \defgroup kos_initflags     Available flags for initialization

    These are the architecture-independent flags that can be specified with
    KOS_INIT_FLAGS.

    \see    dreamcast_initflags
    @{
*/
/** \brief  Default init flags (IRQs on, preemption enabled). */
#define INIT_DEFAULT \
    (INIT_IRQ | INIT_THD_PREEMPT)

#define INIT_NONE           0x0000  /**< \brief Don't init optional things */
#define INIT_IRQ            0x0001  /**< \brief Enable IRQs at startup */
/* Preemptive mode is the only mode now. Keeping define for compatability. */
#define INIT_THD_PREEMPT    0x0002  /**< \brief Enable thread preemption */
#define INIT_NET            0x0004  /**< \brief Enable built-in networking */
#define INIT_MALLOCSTATS    0x0008  /**< \brief Enable malloc statistics */
#define INIT_QUIET          0x0010  /**< \brief Disable dbgio */
/** @} */

__END_DECLS

#endif /* !__KOS_INIT_H */
