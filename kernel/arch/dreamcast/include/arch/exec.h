/* KallistiOS ##version##

   arch/dreamcast/include/arch/exec.h
   (c)2002 Megan Potter

*/

/** \file    arch/exec.h
    \brief   Program execution.
    \ingroup system_overlays

    This file contains functions that allow you to replace the currently running
    program with another binary that has already been loaded into memory. Doing
    so is expected to replace the currently running binary in its entirety, and
    these functions do not return to the calling function.

    \author Megan Potter
*/

#ifndef __ARCH_EXEC_H
#define __ARCH_EXEC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \defgroup system_overlays   Overlays
    \brief                      API for loading and executing overlays
    \ingroup                    system

    @{
*/

/** \brief  Replace the currently running binary.

    This function will replace the currently running binary with whatever is
    at the specified address. This function does not return.

    \param  image           The binary to run (already loaded into RAM).
    \param  length          The length of the binary.
    \param  address         The address of the binary's starting point.
*/
void arch_exec_at(const void *image, uint32_t length, uint32_t address) __noreturn;

/** \brief  Replace the currently running binary at the default address.

    This is a convenience function for arch_exec_at() that assumes that the
    binary has been set up with its starting point at the standard location.
    In the case of the Dreamcast, this is 0xAC010000 (or 0x8C010000, in P1).

    \param  image           The binary to run (already loaded into RAM).
    \param  length          The length of the binary.
*/
void arch_exec(const void *image, uint32_t length) __noreturn;

/** @} */

__END_DECLS

#endif  /* __ARCH_EXEC_H */

