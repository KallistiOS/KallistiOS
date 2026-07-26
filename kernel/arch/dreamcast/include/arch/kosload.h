/* KallistiOS ##version##

   arch/dreamcast/include/arch/kosload.h
   Copyright (C) 2025 Donald Haase
   Copyright (C) 2026 Andy Barajas

*/

/** \file    arch/kosload.h
    \brief   Dreamcast-specific kos-load addresses.
    \ingroup vfs_kosload

    Defines the syscall vector address and magic address for dc-load on the
    Dreamcast.  Included automatically by kernel/fs/fs_kosload.c via the
    arch-specific include path; do not include directly.
*/

#ifndef __ARCH_KOSLOAD_H
#define __ARCH_KOSLOAD_H

#include <dc/fifo.h>
#include <dc/memory.h>

/** \brief  Address of the dc-load syscall function pointer */
#define VEC_KOSLOAD      (MEM_AREA_P1_BASE | 0x0C004008)

/** \brief  Address of the dc-load magic value in BIOS RAM */
#define KOSLOADMAGICADDR ((unsigned int *)0x8c004004)

/** \brief  Flush the SH4 FIFO before issuing a syscall (serial mode) */
#define KOSLOAD_ARCH_FIFO_FLUSH()  do { while(FIFO_STATUS & FIFO_SH4); } while(0)

#endif  /* __ARCH_KOSLOAD_H */
