/* KallistiOS ##version##

   kernel/arch/dreamcast/include/arch/memory.h
   Copyright (C) 2023 Donald Haase

*/

/** \file    arch/memory.h
    \brief   Constants for areas of the system memory map.
    \ingroup memory

    Various addresses and masks that are set by the SH7750. None of the values
    here are Dreamcast-specific.

    These values are drawn from the Hitatchi SH7750 Series Hardware Manual rev 6.0.

    \author Donald Haase
*/

#ifndef __ARCH_MEMORY_H
#define __ARCH_MEMORY_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \defgroup memory Address Space
    \brief    Basics of the SH4 Memory Map
    \ingroup  system

    The SH7750 Series physical address space is mapped onto a 29-bit external
    memory space, with the upper 3 bits of the address indicating which memory
    region will be used. The P0/U0 memory region spans a 2GB space with the
    bottom 512MB mirrored to the P1, P2, and P3 regions.

*/

/** \brief Mask a cache-agnostic address.
    \ingroup memory

    This masks out the upper 3 bits of an address. This is used when it is
    necessary to access memory with a specified caching mode. This is needed for
    DMA and SQ usage as well as various MMU functions.

*/
#define MEM_AREA_CACHE_MASK 0x1fffffff

/** \brief U0 memory region base (cacheable).
    \ingroup memory

    This is the base user mode memory address. It is cacheable as determined
    by the WT bit of the cache control register. By default KOS sets this to
    copy-back mode.

    KOS runs in privileged mode, so this is here merely for completeness.

*/
#define MEM_AREA_U0_BASE    0x00000000

/** \brief P0 memory region base (cacheable).
    \ingroup memory

    This is the base privileged mode memory address. It is cacheable as determined
    by the WT bit of the cache control register. By default KOS sets this to
    copy-back mode.

*/
#define MEM_AREA_P0_BASE    0x00000000

/** \brief P1 memory region base (cacheable).
    \ingroup memory

    This is a modularly cachable memory region. It is cacheable as determined by
    the CB bit of the cache control register. That allows it to function in a
    different caching mode (copy-back v write-through) than the U0, P0, and P3
    regions, whose cache mode are governed by the WT bit. By default KOS sets this
    to the same copy-back mode as the other cachable regions.

*/
#define MEM_AREA_P1_BASE    0x80000000

/** \brief P2 memory region base (non-cacheable).
    \ingroup memory

    This is the non-cachable memory region. It is most frequently for DMA
    transactions to ensure reads are not cached.

*/
#define MEM_AREA_P2_BASE    0xa0000000

/** \brief P3 memory region base (cacheable).
    \ingroup memory

    This functions as the lower 512MB of P0.

*/
#define MEM_AREA_P3_BASE    0xc0000000

/** \brief P4 memory region base (non-cacheable)
    \ingroup memory

    This offset maps to on-chip I/O channels.
*/
#define MEM_AREA_P4_BASE    0xe0000000

/** \defgroup p4mem     P4 memory region
    \brief              P4 SH-internal memory region (non-cacheable).
    \ingroup            memory
*/

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

/** \defgroup sh4_cr_regs   Control Registers
    \brief                  Addresses of control registers within the P4 area
    \ingroup                p4mem
*/

/** \defgroup sh4_mmu_regs MMU
    \brief MMU Control Registers
    \ingroup sh4_cr_regs

    \see arch\mmu.h

    These are registers for controlling the MMU as defined in table 3.1
    of Hitatchi SH7750 Series Hardware Manual rev 6.0, titled "MMU Registers".
    All are accessed as 32-bit values.

*/

/** \brief MMU Page table entry high.
    \ingroup sh4_mmu_regs

    When an MMU exception or address error exception occurs, the virtual page number (VPN)
    (the upper 22-bits of the virtual address causing the exception) is written
    to the register. The bottom 8 bits of the register are software-fillable as
    an 8 bit ID (ASID) of the process causing the exception.
*/
#define SH4_REG_MMU_PTEH                      0xff000000

/** \brief MMU Page table entry low.
    \ingroup sh4_mmu_regs

    Holds the physical page number (PPN) in bits 10-28 and page management flags in 0-8.
*/
#define SH4_REG_MMU_PTEL                      0xff000004

/** \brief MMU Translation table base.
    \ingroup sh4_mmu_regs

    Holds the base address of the currently used page table.
*/
#define SH4_REG_MMU_TTB                       0xff000008

/** \brief MMU TLB Exception address.
    \ingroup sh4_mmu_regs

    After an MMU exception or address error exception occurs, the virtual address where the
    exception occurred is stored here.
*/
#define SH4_REG_MMU_TEA                       0xff00000c

/** \brief MMU Control Register.
    \ingroup sh4_mmu_regs

    Holds configuration values including enable/disable of MMU Address Translation (at bit 0)
*/
#define SH4_REG_MMU_CR                        0xff000010

/** \brief MMU Page table entry assistance.
    \ingroup sh4_mmu_regs

    Stores assistance bits for PCMCIA access to the UTLB via LDTLB. This is currently unused by KOS.
*/
#define SH4_REG_MMU_PTEA                      0xff000034

/** \defgroup sh4_ubc_regs UBC
    \brief UBC Control Registers
    \ingroup sh4_cr_regs

    \see dc\ubc.h

    These are registers for controlling the UBC as defined in table 20.1
    of Hitatchi SH7750 Series Hardware Manual rev 6.0, titled "UBC Registers"

*/

/** \brief UBC Break ASID register A
    \ingroup sh4_ubc_regs

    Specifies the ASID used in the channel A break condition. 8-bit RW.
*/
#define SH4_REG_UBC_BASRA                     0xff000014

/** \brief UBC Break ASID register B
    \ingroup sh4_ubc_regs

    Specifies the ASID used in the channel B break condition. 8-bit RW.
*/
#define SH4_REG_UBC_BASRB                     0xff000018

/** \brief UBC Break address register A
    \ingroup sh4_ubc_regs

    Specifies the virtual address used in the channel A break conditions. 32-bit RW.
*/
#define SH4_REG_UBC_BARA                      0xff200000

/** \brief UBC Break address mask register A
    \ingroup sh4_ubc_regs

    Specifies the settings for masking the ASID in channel A. 8-bit RW.
*/
#define SH4_REG_UBC_BAMRA                     0xff200004

/** \brief UBC Break bus cycle register A
    \ingroup sh4_ubc_regs

    Sets three conditions: 1) instruction/operand access 2) RW 3) Operand size. 16-bit RW.
*/
#define SH4_REG_UBC_BBRA                      0xff200008

/** \brief UBC Break address register B
    \ingroup sh4_ubc_regs

    Specifies the virtual address used in the channel B break conditions. 32-bit RW.
*/
#define SH4_REG_UBC_BARB                      0xff20000c

/** \brief UBC Break address mask register B
    \ingroup sh4_ubc_regs

    Specifies the settings for masking the ASID in channel B. 8-bit RW.
*/
#define SH4_REG_UBC_BAMRB                     0xff200010

/** \brief UBC Break bus cycle register B
    \ingroup sh4_ubc_regs

    Sets three conditions: 1) instruction/operand access 2) RW 3) Operand size. 16-bit RW.
*/
#define SH4_REG_UBC_BBRB                      0xff200014


/** \brief UBC Break data register B
    \ingroup sh4_ubc_regs

    Specifies the data to be used in the channel B break conditions. 32-bit RW.
    Currently unused by KOS
*/
#define SH4_REG_UBC_BDRB                      0xff200018

/** \brief UBC Break mask register B
    \ingroup sh4_ubc_regs

    Specifies which bits of the break data set in SH4_REG_UBC_BDRB are to be masked. 32-bit RW.
    Currently unused by KOS
*/
#define SH4_REG_UBC_BDMRB                     0xff20001c

/** \brief UBC Break control register
    \ingroup sh4_ubc_regs

    Specifies various settings for UBC as well as condition match flags. 16-bit RW.
*/
#define SH4_REG_UBC_BRCR                      0xff200020

__END_DECLS

#endif /* __ARCH_MEMORY_H */
