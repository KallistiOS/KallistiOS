/* KallistiOS ##version##

   arch/xbox/include/arch/exec.h
   Copyright (C) 2026 The KallistiOS Contributors
*/

/** \file    arch/exec.h
    \brief   Xbox program overlay interface.

    The declarations are part of KOS's architecture header contract. The Xbox
    in-place overlay backend is not implemented yet. Ordinary termination
    follows the saved loader return path for hosted guests or the isolated
    firmware-return thunk for a native XBE.
*/

#ifndef __ARCH_XBOX_EXEC_H
#define __ARCH_XBOX_EXEC_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

void arch_exec_at(const void *image, uint32_t length,
                  uint32_t address) __noreturn;
void arch_exec(const void *image, uint32_t length) __noreturn;

__END_DECLS

#endif /* __ARCH_XBOX_EXEC_H */
