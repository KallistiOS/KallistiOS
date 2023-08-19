/* KallistiOS ##version##

   kernel/arch/dreamcast/include/arch/memory.h
   Copyright (C) 2023 Donald Haase

*/

/** \file   arch/memory.h
    \brief  Constants for areas of the system memory map.

    Various addresses and masks that are set by the SH7750. None of the values 
    here are Dreamcast-specific.

    These values are drawn from the Hitatchi SH7750 Series Hardware Manual rev 6.0.

    \author Donald Haase
*/

#ifndef __ARCH_MEMORY_H
#define __ARCH_MEMORY_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \defgroup memory Memory
    \brief    Basics of the SH4 Memory Map

    The SH7750 Series physical address space is mapped onto a 29-bit external 
    memory space, with the upper 3 bits of the address indicating which memory 
    region will be used. The P0/U0 memory region spans a 2GB space with the 
    bottom 512MB mirrored to the P1, P2, and P3 regions.

*/

/** \brief Mask a cache-agnostic address.
    \ingroup memory

    This masks out the upper 3 bits of an address. This is used when it is 
    necssary to access memory with a specified caching mode. This is needed for 
    DMA and SQ usage as well as various MMU functions.

*/
#define MEM_AREA_CACHE_MASK 0x1fffffff

/** \brief U0 memory region (cachable).
    \ingroup memory

    This is the base user mode memory address. It is cacheable as determined 
    by the WT bit of the cache control register. By default KOS sets this to 
    copy-back mode.

    KOS runs in privileged mode, so this is here merely for completeness.

*/
#define MEM_AREA_U0_BASE    0x00000000

/** \brief P0 memory region (cachable).
    \ingroup memory

    This is the base privileged mode memory address. It is cacheable as determined 
    by the WT bit of the cache control register. By default KOS sets this to 
    copy-back mode.

*/
#define MEM_AREA_P0_BASE    0x00000000

/** \brief P1 memory region (cachable).
    \ingroup memory

    This is a modularly cachable memory region. It is cacheable as determined by 
    the CB bit of the cache control register. That allows it to function in a 
    different caching mode (copy-back v write-through) than the U0, P0, and P3 
    regions, whose cache mode are governed by the WT bit. By default KOS sets this 
    to the same	copy-back mode as the other cachable regions.

*/
#define MEM_AREA_P1_BASE    0x80000000

/** \brief P2 memory region (non-cachable).
    \ingroup memory

    This is the non-cachable memory region. It is most frequently for DMA 
    transactions to ensure reads are not cached.

*/
#define MEM_AREA_P2_BASE    0xa0000000

/** \brief P3 memory region (cachable).
    \ingroup memory

    This functions as the lower 512MB of P0.

*/
#define MEM_AREA_P3_BASE    0xc0000000

/** \brief P4 SH-internal memory region (non-cachable).
    \defgroup p4mem
    \ingroup memory

    This offset maps to on-chip I/O channels.

*/
#define MEM_AREA_P4_BASE    0xe0000000

/** \brief Store Queue (SQ) memory base.
    \ingroup p4mem

    This offset maps to the SQ memory region. RW to addresses from 
    0xe0000000-0xe3ffffff follow SQ rules.

    \see dc\sq.h

*/
#define MEM_AREA_SQ_BASE    0xe0000000

/** \brief Instruction cache address array base.
    \ingroup p4mem

    This offset is used for direct access to the instruction cache address array.

*/
#define MEM_AREA_ICACHE_ADDRESS_ARRAY_BASE    0xf0000000

/** \brief Instruction cache data array base.
    \ingroup p4mem

    This offset is used for direct access to the instruction cache data array.

*/
#define MEM_AREA_ICACHE_DATA_ARRAY_BASE       0xf1000000

/** \brief Instruction TLB address array base.
    \ingroup p4mem

    This offset is used for direct access to the instruction TLB address array.

*/
#define MEM_AREA_ITLB_ADDRESS_ARRAY_BASE      0xf2000000

/** \brief Instruction TLB data array 1 base.
    \ingroup p4mem

    This offset is used for direct access to the instruction TLB data array 1.

*/
#define MEM_AREA_ITLB_DATA_ARRAY1_BASE        0xf3000000

/** \brief Instruction TLB data array 2 base.
    \ingroup p4mem

    This offset is used for direct access to the instruction TLB data array 2.

*/
#define MEM_AREA_ITLB_DATA_ARRAY2_BASE        0xf3800000

/** \brief Operand cache address array base.
    \ingroup p4mem

    This offset is used for direct access to the operand cache address array.

*/
#define MEM_AREA_OCACHE_ADDRESS_ARRAY_BASE    0xf4000000

/** \brief Instruction cache data array base.
    \ingroup p4mem

    This offset is used for direct access to the operand cache data array.

*/
#define MEM_AREA_OCACHE_DATA_ARRAY_BASE       0xf5000000

/** \brief Unified TLB address array base.
    \ingroup p4mem

    This offset is used for direct access to the unified TLB address array.

*/
#define MEM_AREA_UTLB_ADDRESS_ARRAY_BASE      0xf6000000

/** \brief Unified TLB data array 1 base.
    \ingroup p4mem

    This offset is used for direct access to the unified TLB data array 1.

*/
#define MEM_AREA_UTLB_DATA_ARRAY1_BASE        0xf7000000

/** \brief Unified TLB data array 2 base.
    \ingroup p4mem

    This offset is used for direct access to the unified TLB data array 2.

*/
#define MEM_AREA_UTLB_DATA_ARRAY2_BASE        0xf7800000

/** \brief Control Register base.
    \ingroup p4mem

    This is the base address of all control registers

*/
#define MEM_AREA_CTRL_REG_BASE                0xff000000

__END_DECLS

#endif /* __ARCH_MEMORY_H */
