/* KallistiOS ##version##

   arch/xbox/include/arch/kosload.h
   Copyright (C) 2026 Andress Barajas

*/

/** \file    arch/kosload.h
    \brief   Xbox-specific kos-load addresses.
    \ingroup vfs_kosload

    Defines the syscall vector address and magic address for kos-load (kos-tool's
    xbox-load-ip) on the original Xbox. Included automatically by
    kernel/fs/fs_kosload.c via the arch-specific include path; do not include
    directly.

    The Xbox loader publishes a small guest-facing header at a fixed low-memory
    address: the magic value at +0 and the syscall function pointer at +4. This
    matches the PlayStation 2 layout (adjacent magic/pointer words) rather than
    the Dreamcast's separate BIOS-RAM addresses; VEC_KOSLOAD is the address of
    the pointer word, which the shared code dereferences once to get the
    callable.
*/

#ifndef __ARCH_KOSLOAD_H
#define __ARCH_KOSLOAD_H

#include <stdint.h>

/** \brief  Address of the kos-load magic value (== KOSLOADMAGICVALUE). */
#define KOSLOADMAGICADDR ((unsigned int *)0x00011000)

/** \brief  Address of the word holding the kos-load syscall function pointer. */
#define VEC_KOSLOAD      ((uintptr_t)0x00011004)

/** \brief  x86 has no serial FIFO to drain before a syscall; no-op. */
#define KOSLOAD_ARCH_FIFO_FLUSH()  do { } while(0)

#endif  /* __ARCH_KOSLOAD_H */
