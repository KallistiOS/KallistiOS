/* KallistiOS ##version##

   arch/dreamcast/include/arch/kosload.h
   Copyright (C) 2025 Donald Haase
   Copyright (C) 2026 Andy Barajas

*/

/** \file    arch/kosload.h
    \brief   Dreamcast-specific kos-load addresses.
    \ingroup vfs_kosload

    Defines the syscall address and magic address for dc-load on the
    Dreamcast.  Included automatically by kernel/kosload.c via the
    arch-specific include path; do not include directly.
*/

#ifndef __ARCH_KOSLOAD_H
#define __ARCH_KOSLOAD_H

#include <stdint.h>

#include <dc/fifo.h>
#include <dc/memory.h>

/** \brief  Size of the dc-load area reserved in bytes (45 KB) */
#define KOSLOAD_SIZE          0xb400

/** \brief  Base address of the dc-load area in RAM */
#define KOSLOAD_BASE_ADDR     0x8c004000

/** \brief  Address of the dc-load magic value in RAM */
#define KOSLOAD_MAGIC_ADDR    (KOSLOAD_BASE_ADDR + 0x4)

/** \brief  Address of the dc-load syscall function pointer */
#define KOSLOAD_SYSCALL_ADDR  (KOSLOAD_BASE_ADDR + 0x8)

/** \brief  Flush the SH4 FIFO before issuing a syscall (serial mode) */
static inline void kosload_flush_fifo(void) {
    while(FIFO_STATUS & FIFO_SH4);
}

#endif  /* __ARCH_KOSLOAD_H */
