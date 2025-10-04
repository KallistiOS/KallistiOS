/* KallistiOS ##version##

   include/kos/linker.h
   Copyright (C) 2025 Paul Cercueil
*/

/** \file    kos/linker.h
    \brief   Linker script related definitions and macros.
    \ingroup linker

    \author Paul Cercueil
*/

#ifndef __KOS_LINKER_H
#define __KOS_LINKER_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

extern uint8_t _bss_start[];
extern uint8_t end[];

__END_DECLS

#endif /* __KOS_LINKER_H */
