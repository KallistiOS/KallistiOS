/* KallistiOS ##version##

   sys/ioctl.h
   Copyright (C) 2024 Andress Barajas
   Copyright (C) 2025 Ruslan Rostovtsev

*/

/** \file    sys/ioctl.h
    \brief   Header for terminal control operations.
    \ingroup vfs_posix

    This file contains definitions for terminal control operations, as specified by
    the POSIX standard. It includes necessary constants and macros for performing
    various control operations on terminals using the ioctl system call.

    \author Andress Barajas
*/

#ifndef __SYS_IOCTL_H
#define __SYS_IOCTL_H

#include <kos/cdefs.h>

__BEGIN_DECLS

#include <kos/fs.h>

#ifndef IOCTL_FS_ROOTBUS_DMA_READY
/** \brief This operation can determine that file system 
 * can read data directly into SPU and PVR RAM's thought the Root Bus
 * and are all the conditions for this met like file position
 * on sector boundary at first reading and DMA aligning for others,
 * if the data stream was not interrupted by another request or seeking.
 * You can also get current alignment requirement in the argument (use uint32_t).
 */
#define IOCTL_FS_ROOTBUS_DMA_READY 0x8001
#endif

/* Define ioctl as an alias for fs_ioctl */
#define ioctl fs_ioctl

__END_DECLS

#endif /* __SYS_IOCTL_H */
