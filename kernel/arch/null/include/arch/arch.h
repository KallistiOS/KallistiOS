/* KallistiOS ##version##

   arch/null/include/arch.h
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

#include <kos/elf.h>

/** \defgroup arch  Architecture
    \brief          Dreamcast Architecture-Specific Options and high-level API
    \ingroup        system
    @{
*/

/** \brief  Top of memory available, depending on memory size. */
#define _arch_mem_top   ((uint32_t) 0x8d000000)

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

/** \brief   Dreamcast specific sleep mode function.
    \ingroup arch
*/
static inline void arch_sleep(void) {
//    __asm__ __volatile__("sleep\n");
}

__END_DECLS

#endif  /* __ARCH_ARCH_H */
