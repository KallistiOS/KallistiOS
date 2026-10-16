/* KallistiOS ##version##

   arch/dreamcast/include/arch/kosload.h
   Copyright (C) 2025 Donald Haase
   Copyright (C) 2026 Andy Barajas

*/

/** \file    arch/kosload.h
    \brief   Dreamcast-specific kos-load memory area.
    \ingroup vfs_kosload

    Describes where dc-load lives in RAM on the Dreamcast, which is all the
    portable driver needs in order to detect the connection.  The native syscall
    transport itself is in arch/dreamcast/hardware/kosload_syscalls.c.

    Included automatically by kernel/kosload.c via the arch-specific include
    path; do not include directly.
*/

#ifndef __ARCH_KOSLOAD_H
#define __ARCH_KOSLOAD_H

#include <stdint.h>

/** \brief  Size of the dc-load area reserved in bytes (45 KB) */
#define KOSLOAD_SIZE          (45 * 1024)

/** \brief  Base address of the dc-load area in RAM */
#define KOSLOAD_BASE_ADDR     0x8c004000

/** \brief  Address of the dc-load magic value in RAM */
#define KOSLOAD_MAGIC_ADDR    (KOSLOAD_BASE_ADDR + 0x4)

#endif  /* __ARCH_KOSLOAD_H */
