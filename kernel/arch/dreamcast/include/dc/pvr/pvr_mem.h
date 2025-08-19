/* KallistiOS ##version##

   dc/pvr/pvr_mem.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2014 Lawrence Sebald
   Copyright (C) 2023 Ruslan Rostovtsev
   Copyright (C) 2024 Falco Girgis
*/

/** \file       dc/pvr/pvr_mem.h
    \brief      VRAM Management and Access
    \ingroup    pvr_vram

    \author Megan Potter
    \author Roger Cattermole
    \author Paul Boese
    \author Brian Paul
    \author Lawrence Sebald
    \author Benoit Miller
    \author Ruslan Rostovtsev
    \author Falco Girgis
*/

#ifndef __DC_PVR_PVR_MEM_H
#define __DC_PVR_PVR_MEM_H

#include <stdint.h>

#include <kos/cdefs.h>
__BEGIN_DECLS

/** \defgroup pvr_vram   VRAM
    \brief               Video memory access and management
    \ingroup             pvr
*/

/** \brief   PVR texture memory pointer.
    \ingroup pvr_vram

    Unlike the old "TA" system, PVR pointers in the new system are actually SH-4
    compatible pointers and can be used directly in place of ta_txr_map().

    Not that anyone probably even remembers the old TA system anymore... 
*/
typedef void *pvr_ptr_t;

/** \defgroup pvr_mem_mgmt   Allocator
    \brief                   Memory management API for VRAM
    \ingroup                 pvr_vram

    PVR memory management in KOS uses a modified dlmalloc; see the
    source file pvr_mem_core.c for more info. 
*/

/** \brief   Allocate a chunk of memory from texture space.
    \ingroup pvr_mem_mgmt

    This function acts as the memory allocator for the PVR texture RAM pool. It
    acts exactly as one would expect a malloc() function to act, returning a
    normal pointer that can be directly written to if one desires to do so. All
    allocations will be aligned to a 32-byte boundary.

    \param  size            The amount of memory to allocate
    
    \return                 A pointer to the memory on success, NULL on error
*/
pvr_ptr_t pvr_mem_malloc(size_t size);

/** \brief   Free a block of allocated memory in the PVR RAM pool.
    \ingroup pvr_mem_mgmt

    This function frees memory previously allocated with pvr_mem_malloc().

    \param  chunk           The location of the start of the block to free
*/
void pvr_mem_free(pvr_ptr_t chunk);

/** \brief   Return the number of bytes available still in the PVR RAM pool.
    \ingroup pvr_mem_mgmt

    \return                 The number of bytes available
*/
size_t pvr_mem_available(void);

/** \brief   Reset the PVR RAM pool.
    \ingroup pvr_mem_mgmt

    This will essentially free any blocks allocated within the pool. There's
    generally not many good reasons for doing this.
*/
void pvr_mem_reset(void);

/** \brief   Print the list of allocated blocks in the PVR RAM pool.
    \ingroup pvr_mem_mgmt

    This function only works if you've enabled KM_DBG in pvr_mem.c.
*/
void pvr_mem_print_list(void);

/** \brief   Print statistics about the PVR RAM pool.
    \ingroup pvr_mem_mgmt

    This prints out statistics like what malloc_stats() provides. Also, if
    KM_DBG is enabled in pvr_mem.c, it prints the list of allocated blocks.
*/
void pvr_mem_stats(void);

__END_DECLS

#endif /* __DC_PVR_PVR_MEM_H */
