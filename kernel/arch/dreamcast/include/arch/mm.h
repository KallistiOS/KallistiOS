/* KallistiOS ##version##

   arch/dreamcast/include/arch/mm.h
   Copyright (C) 2024 Falco Girgis

*/

/** \file    arch/mm.h
    \brief   Public API for General Memory Management
    \ingroup arch_mm

    This file contains the public API for general memory management,
    including the program break and data segments.

    \author Falco Girgis
*/

#ifndef __ARCH_MM_H
#define __ARCH_MM_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

/** \defgroup arch_mm  Memory Management
    \brief    API for managing program memory segments.
    @{
*/

/** \defgroup mm_pages MMU Page Constants
    \brief    Constants and defines for MMU-mapped pages.
    @{
*/

#define PAGESIZE        4096            /**< \brief Page size (for MMU) */
#define PAGESIZE_BITS   12              /**< \brief Bits for page size */
#define PAGEMASK        (PAGESIZE - 1)  /**< \brief Mask for page offset */

/** \brief Base address of available physical pages. */
#define page_phys_base  0x8c010000

/** \brief Number of physical pages spanning memory. */
#define page_count      ((_arch_mem_top - page_phys_base) / PAGESIZE)

/** @} */

/** \defgroup mm_ram Physical RAM Constants
    \brief    Constants and defines for physical RAM regions and sizes.
    @{
*/

/** \brief Beginning of RAM, fixed location. */
#define _arch_mem_bottom    ((uint32_t)0x8c000000)

/** \brief End of RAM, depending on memory size. */
#ifdef __KOS_GCC_32MB__
    extern uint32_t _arch_mem_top;
#else
#   pragma message "Outdated toolchain: Update for 32MB RAM support!"
#   define _arch_mem_top   ((uint32_t)0x8d000000)
#endif

/** \brief Total bytes of physical memory installed on the machine. */
#define HW_MEMSIZE (_arch_mem_top - _arch_mem_bottom)

/** \brief Determine whether system has 32MB double RAM expansion. */
#define DBL_MEM (!!(_arch_mem_top - 0x8d000000))

/** \defgroup hw_memsizes Memory Capacitites
    \brief    Total bytes of memory for each RAM size.
    @{
*/

#define HW_MEM_16  16777216   /**< \brief 16M retail Dreamcast */
#define HW_MEM_32  33554432   /**< \brief 32M NAOMI/modded Dreamcast */

/** @} */

/** \brief Total size allocated for kernel thread's stack. */
#define MM_KERNEL_STACK_SIZE 65536

/** @} */

/** \name  RAM 
    \brief Methods pertaining to total system memory.
    @{
*/

/** \brief First address of physical memory.

    \returns    Pointer to the first address.

    \sa mm_ram_end()
*/
void *mm_ram_start(void);

/** \brief Last address of physical memory. 

    \returns    Pointer to the last address.

    \sa mm_ram_start()
*/
void *mm_ram_end(void);

/** \brief Total number or used bytes of physical memory.

    \returns    Number of bytes used.

    \sa mm_ram_free()
*/
size_t mm_ram_used(void);

/** \brief Total number of available bytes of physical memory.
 
    \returns    Number of available bytes.

    \sa mm_ram_end()
*/
size_t mm_ram_free(void);

/** @} */

/** \name  Program Stack 
    \brief Methods pertaining to the main kernel thread's stack.
    @{
*/

/** \brief First address within the main thread's stack. 

    \returns    First stack address.

    \sa mm_stack_current(), mm_stack_end()
*/
void *mm_stack_start(void);

/** \brief Current stack pointer for main thread.

    \returns    Current stack address.

    \sa mm_stack_start(), mm_stack_end()
*/
void *mm_stack_current(void);

/** \brief Last valid stack address of main thread.

    \returns    Last stack address.

    \sa mm_stack_start(), mm_stack_current()
*/
void *mm_stack_end(void);

/** \brief Bytes used within main thread's stack.

    \returns    Bytes used.

    \sa mm_stack_free()
*/
size_t mm_stack_used(void);

/** \brief Available bytes within main thread's stack.

    \returns    Bytes available.

    \sa mm_stack_used()
*/
size_t mm_stack_free(void);

/** @} */

/** \name  Program Break Segment 
    \brief Methods for configuring and querying the program break segment.
    @{
*/

/** \brief Program break start.
 
    First address of the program break segment. This is fixed.

    \returns    Pointer to the start address.

    \sa mm_brk_current(), mm_brk_end()
*/
void *mm_brk_start(void);

/** \brief Program break current position.

    Last address currently allocated (to the heap) within the program break
    segment. 

    \returns    Pointer to the current address.

    \sa mm_brk_start(), mm_brk_end(), mm_sbrk()
*/
void *mm_brk_current(void);

/** \brief Program break end.
 
    Final valid address for the program break segment when it has reached
    maximum capacity.

    \returns    Pointer to the last address.

    \sa mm_brk_start(), mm_brk_current(), mm_brk_set_capacity()
*/
void *mm_brk_end(void);

/** \brief Program break remaining bytes.

     The total number of bytes which may yet still be allocated to the program
     break segment before it reaches its full capacity.

    \note
    This value is simply calculated as mm_brk_end() - mm_brk_current().

    \returns   Free bytes.

    \sa mm_brk_used(), mm_brk_set_capacity()
*/
size_t mm_brk_free(void);

/** \brief Program break allocated bytes. 

    The total number of bytes which have already been allocated to the program
    break segment (for the heap).

    \note
    This value is simply calculated as mm_brk_current() - mm_brk_start().

    \returns    Used bytes.

    \sa mm_brk_free(), mm_brk_set_capacity()
*/
size_t mm_brk_used(void);

/** \brief Maximum capacity of the program break segment.
 
    The largest number of bytes which can be used by the program break segment
    without stomping on the main thread's stack in RAM.

    \note
    This size is used as the default capacity for the user break segment. Check
    mm_brk_set_capacity() to see how to resize it.

    \returns    Total maximum capacity in bytes

    \sa mm_brk_set_capacity()
*/
size_t mm_brk_max_capacity(void);

/** \brief Sets the capacity of the progra break segment.

    The capacity is the total number of bytes which can be allocated via
    mm_sbrk(). It is calculated as mm_brk_end() - mm_brk_start().

    \note
    Setting a capacity that is less than the maximum capacity will create an
    unused "reserved" segment following the program break segment. It can be
    accessed via mm_reserved_start() and mm_reserved_end().

    \warning
    Once mm_brk_current() reaches mm_brk_end(), exhausting its entire capacity,
    any further requests for heap memory or calls to mm_sbrk() will fail.

    \param  bytes   New capacity in bytes.

    \retval 0       Success.
    \retval -1      Failure: \p bytes is greater than mm_brk_max_capacity().
    \retval -2      Failure: \p bytes is less than the number of in-use bytes.

    \sa mm_brk_max_capacity()
*/
int mm_brk_set_capacity(size_t bytes);

/** \brief Sets the current program break segment position.

    Attempts to set the mm_brk_current() pointer directly.

    \param new_pos  New current program break position pointer.

    \retval 0       success: 
    \retval -1      Failure: (`errno` is set to `ENOMEM`)
                        - \p new_pos < mm_brk_start()
                        - \p new_pos > mm_brk_end()

    \sa mm_sbrk()

*/
int mm_brk(void *new_pos);

/** \brief Resizes the program break segment. 

    Attempts to resize the program break segment by \p increment,
    either shrinking or growing as needed.

    \param increment    Offset to add to mm_sbrk_current()

    \returns            Success: Previous mm_sbrk_current() value returned.
    \retval     -1      Failure: (`errno` is set to `ENOMEM`)
                            - mm_brk_current() + \p increment > mm_brk_end().
                            - mm_brk_current() + \p increment < mm_brk_start().
*/
void *mm_sbrk(intptr_t increment);

/** @} */

/** \name Reserved Segment 
    \brief Methods for accessing the optional extra segment.
    @{
*/

/** \brief Returns the first address of the reserved segment.
 
    If there is a reserved segment (the program break segment's capacity is
    less than its maximum capacity), its first address is returned.

    \returns            Pointer to the first address.
    \retval     NULL    No reserved segment is present.

    \sa mm_reserved_end(), mm_brk_set_capacity()
*/
void *mm_reserved_start(void);

/** \brief Returns the last address of the reserved segment.

    If there is a reserved segment (the program break segment's capacity is
    less than its maximum capacity), its last address is returned.

    \returns            Pointer to the last address.
    \retval     NULL    No reserved segment is present.
*/
void *mm_reserved_end(void);

/** @} */

/** \cond  INTERNAL 
    \brief Initializes the memory management routines.

    This is called automatically by KOS at startup

    \retval 0   Success
*/
int mm_init(void);
/** \endcond */

/** @} */

__END_DECLS

#endif

