/* KallistiOS ##version##

   arch/dreamcast/include/arch/exec.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Falco Girgis
*/

/** \file    arch/exec.h
    \brief   Program execution.
    \ingroup system_overlays

    Public API for loading and executing system overlays.

    \author Megan Potter
*/

#ifndef __ARCH_EXEC_H
#define __ARCH_EXEC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \defgroup system_overlays   Overlays
    \brief                      API for loading and executing overlays
    \ingroup                    system

    This file contains functions that allow you to replace the currently running
    program with another binary that has already been loaded into memory. Doing
    so is expected to replace the currently running binary in its entirety, and
    these functions do not return to the calling function.

    @{
*/

uintptr_t arch_exec_address(void);

void arch_exec_with_args(const void *image, size_t length, ...) __noreturn;

void arch_exec_at_with_args(const void *image, size_t length, uintptr_t address, ...) __noreturn;

/** \brief  Replace the currently running binary.

    This function will replace the currently running binary with whatever is
    at the specified address. 
    
    \note
    This function does not return.

    \param  image           The binary to run (already loaded into RAM).
    \param  length          The length of the binary.
    \param  address         The address of the binary's starting point.

    \sa arch_exec()
*/
void arch_exec_at(const void *image, size_t length, uintptr_t address) __noreturn;

/** \brief  Replace the currently running binary at the default address.

    This is a convenience function for arch_exec_at() that assumes that the
    binary has been set up with its starting point at the standard location.

    \note
    In the case of the Dreamcast, this standard location is 0xAC010000
    (or `_executable_start` or 0x8C010000, in P1).

    \param  image           The binary to run (already loaded into RAM).
    \param  length          The length of the binary.

    \sa arch_exec_at()
*/
void arch_exec(const void *image, size_t length) __noreturn;

/** @} */

__END_DECLS

#endif  /* __ARCH_EXEC_H */

