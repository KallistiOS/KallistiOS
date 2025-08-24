/* KallistiOS ##version##

   arch/dreamcast/include/arch.h
   Copyright (C) 2001 Megan Potter
   Copyright (C) 2013, 2020 Lawrence Sebald

*/

/** \file    arch/arch.h
    \brief   Dreamcast architecture specific options.
    \ingroup arch

    This file has various architecture specific options defined in it.

    \author Megan Potter
*/

#ifndef __ARCH_ARCH_H
#define __ARCH_ARCH_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdbool.h>

#include <arch/types.h>
#include <kos/elf.h>

/** \defgroup arch  Architecture
    \brief          Dreamcast Architecture-Specific Options and high-level API
    \ingroup        system
    @{
*/

/** \brief  Top of memory available, depending on memory size. */
#if defined(__KOS_GCC_32MB__) || __KOS_GCC_PATCHLEVEL__ >= 2025062800
extern uint32 _arch_mem_top;
#else
#pragma message "Outdated toolchain: not patched for 32MB support, limiting "\
    "KOS to 16MB-only behavior to retain maximum compatibility. Please "\
    "update your toolchain."
#define _arch_mem_top   ((uint32) 0x8d000000)
#endif

/** \brief  Start and End address for .text portion of program. */
extern char _executable_start;
extern char _etext;

#define PAGESIZE        4096            /**< \brief Page size (for MMU) */
#define PAGESIZE_BITS   12              /**< \brief Bits for page size */
#define PAGEMASK        (PAGESIZE - 1)  /**< \brief Mask for page offset */

/** \brief  Page count "variable".

    The number of pages is static, so we can optimize this quite a bit. */
#define page_count      ((_arch_mem_top - page_phys_base) / PAGESIZE)

/** \brief  Base address of available physical pages. */
#define page_phys_base  0x8c010000

#ifndef THD_SCHED_HZ
/** \brief Scheduler interrupt frequency

    Timer interrupt frequency for the KOS thread scheduler.

    \note
    This value is what KOS uses initially upon startup, but it can be
    reconfigured at run-time.

    \sa thd_get_hz(), thd_set_hz()
*/
#define THD_SCHED_HZ    100
#endif

/** Legacy symbol for scheduler frequency.
 *  \deprecated
 *  \sa THD_SCHED_HZ
 */
static const
unsigned HZ __depr("Please use the new THD_SCHED_HZ macro.") = THD_SCHED_HZ;

/** \brief  Global symbol prefix in ELF files. */
#define ELF_SYM_PREFIX      "_"

/** \brief  Length of global symbol prefix in ELF files. */
#define ELF_SYM_PREFIX_LEN  1

/** \brief  Standard name for this arch. */
#define ARCH_NAME           "Dreamcast"

/** \brief  ELF class for this architecture. */
#define ARCH_ELFCLASS       ELFCLASS32

/** \brief  ELF data encoding for this architecture. */
#define ARCH_ELFDATA        ELFDATA2LSB

/** \brief  ELF machine type code for this architecture. */
#define ARCH_CODE           EM_SH

/** \brief  Panic function.

    This function will cause a kernel panic, printing the specified message.

    \param  str             The error message to print.
    \note                   This function will never return!
*/
void arch_panic(const char *str) __noreturn;

/** \brief  Kernel C-level entry point.
    \note                   This function will never return!
*/
void arch_main(void) __noreturn;

/** @} */

/** \defgroup arch_retpaths Exit Paths
    \brief                  Potential exit paths from the kernel on
                            arch_exit()
    \ingroup                arch
    @{
*/
#define ARCH_EXIT_RETURN    1   /**< \brief Return to loader */
#define ARCH_EXIT_MENU      2   /**< \brief Return to system menu */
#define ARCH_EXIT_REBOOT    3   /**< \brief Reboot the machine */
/** @} */

/** \brief   Set the exit path.
    \ingroup arch

    The default, if you don't call this, is ARCH_EXIT_RETURN.

    \param  path            What arch_exit() should do.
    \see    arch_retpaths
*/
void arch_set_exit_path(int path);

/** \brief   Generic kernel "exit" point.
    \ingroup arch
    \note                   This function will never return!
*/
void arch_exit(void) __noreturn;

/** \brief   Kernel "return" point.
    \ingroup arch
    \note                   This function will never return!
*/
void arch_return(int ret_code) __noreturn;

/** \brief   Kernel "abort" point.
    \ingroup arch
    \note                   This function will never return!
*/
void arch_abort(void) __noreturn;

/** \brief   Kernel "reboot" call.
    \ingroup arch
    \note                   This function will never return!
*/
void arch_reboot(void) __noreturn;

/** \brief   Kernel "exit to menu" call.
    \ingroup arch
    \note                   This function will never return!
*/
void arch_menu(void) __noreturn;

/** \defgroup hw_memsizes           Memory Capacity
    \brief                          Console memory sizes
    \ingroup                        arch

    These are the various memory sizes, in bytes, that can be returned by the
    HW_MEMSIZE macro.

    @{
*/
#define HW_MEM_16           16777216   /**< \brief 16M retail Dreamcast */
#define HW_MEM_32           33554432   /**< \brief 32M NAOMI/modded Dreamcast */
/** @} */

/** \brief   Determine how much memory is installed in current machine.
    \ingroup arch

    \return The total size of system memory in bytes.
*/
#define HW_MEMSIZE (_arch_mem_top - 0x8c000000)

/** \brief   Use this macro to easily determine if system has 32MB of RAM.
    \ingroup arch

    \return Non-zero if console has 32MB of RAM, zero otherwise
*/
#define DBL_MEM (_arch_mem_top - 0x8d000000)

/* These are in mm.c */
/** \brief   Initialize the memory management system.
    \ingroup arch

    \retval 0               On success (no error conditions defined).
*/
int mm_init(void);

/** \brief   Request more core memory from the system.
    \ingroup arch

    \param  increment       The number of bytes requested.
    \return                 A pointer to the memory.
    \note                   This function will panic if no memory is available.
*/
void * mm_sbrk(unsigned long increment);

/* Bring in the init flags for compatibility with old code that expects them
   here. */
#include <kos/init.h>

/* Dreamcast-specific arch init things */
/** \brief   Jump back to the bootloader.
    \ingroup arch

    You generally shouldn't use this function, but rather use arch_exit() or
    exit() instead.

    \note                   This function will never return!
*/
void arch_real_exit(int ret_code) __noreturn;

/** \brief   Initialize bare-bones hardware systems.
    \ingroup arch

    This will be done automatically for you on start by the default arch_main(),
    so you shouldn't have to deal with this yourself.

    \retval 0               On success (no error conditions defined).
*/
int hardware_sys_init(void);

/** \brief   Initialize some peripheral systems.
    \ingroup arch

    This will be done automatically for you on start by the default arch_main(),
    so you shouldn't have to deal with this yourself.

    \retval 0               On success (no error conditions defined).
*/
int hardware_periph_init(void);

/** \brief   Shut down hardware that was initted.
    \ingroup arch

    This function will shut down anything initted with hardware_sys_init() and
    hardware_periph_init(). This will be done for you automatically by the
    various exit points, so you shouldn't have to do this yourself.
*/
void hardware_shutdown(void);

/** \defgroup hw_consoles           Console Types
    \brief                          Byte values returned by hardware_sys_mode()
    \ingroup  arch

    These are the various console types that can be returned by the
    hardware_sys_mode() function.

    @{
*/
#define HW_TYPE_RETAIL      0x0     /**< \brief A retail Dreamcast. */
#define HW_TYPE_SET5        0x9     /**< \brief A Set5.xx devkit. */
/** @} */

/** \defgroup hw_regions            Region Codes
    \brief                          Values returned by hardware_sys_mode();
    \ingroup  arch

    These are the various region codes that can be returned by the
    hardware_sys_mode() function. 

    \note
    A retail Dreamcast will always return 0 for the region code. 
    You must read the region of a retail device from the flashrom.

    \see    fr_region
    \see    flashrom_get_region()

    @{
*/
#define HW_REGION_UNKNOWN   0x0     /**< \brief Unknown region. */
#define HW_REGION_ASIA      0x1     /**< \brief Japan/Asia (NTSC) */
#define HW_REGION_US        0x4     /**< \brief North America */
#define HW_REGION_EUROPE    0xC     /**< \brief Europe (PAL) */
/** @} */

/** \brief   Retrieve the system mode of the console in use.
    \ingroup arch

    This function retrieves the system mode register of the console that is in
    use. This register details the actual system type in use (and in some system
    types the region of the device).

    \param  region          On return, the region code (one of the
                            \ref hw_regions) of the device if the console type
                            allows reading it through the system mode register
                            -- otherwise, you must retrieve the region from the
                            flashrom.
    \return                 The console type (one of the \ref hw_consoles).
*/
int hardware_sys_mode(int *region);

/** \brief   Dreamcast specific sleep mode function.
    \ingroup arch
*/
static inline void arch_sleep(void) {
    __asm__ __volatile__("sleep\n");
}

/** \brief   Returns true if the passed address is likely to be valid. Doesn't
             have to be exact, just a sort of general idea.
    \ingroup arch

    \return                 Whether the address is valid or not for normal
                            memory access.
*/
static inline bool arch_valid_address(uintptr_t ptr) {
    return ptr >= 0x8c010000 && ptr < _arch_mem_top;
}

/** \brief   Returns true if the passed address is in the text section of your
             program.
    \ingroup arch

    \return                 Whether the address is valid or not for text
                            memory access.
*/
static inline bool arch_valid_text_address(uintptr_t ptr) {
    return ptr >= (uintptr_t)&_executable_start && ptr < (uintptr_t)&_etext;
}

__END_DECLS

#endif  /* __ARCH_ARCH_H */
