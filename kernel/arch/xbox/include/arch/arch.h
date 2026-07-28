/* KallistiOS ##version##

   arch/xbox/include/arch/arch.h
   Copyright (C) 2026 The KallistiOS Contributors
*/

/** \file    arch/arch.h
    \brief   Original Xbox architecture definitions.
    \ingroup arch
*/

#ifndef __ARCH_ARCH_H
#define __ARCH_ARCH_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdbool.h>
#include <stdint.h>

#include <kos/elf.h>
#include <kos/linker.h>

/** \defgroup arch Xbox Architecture
    \ingroup system
    @{
*/

#define PAGESIZE       4096U           /**< \brief Native x86 page size */
#define PAGESIZE_BITS  12U             /**< \brief Bits in a page offset */
#define PAGEMASK       (PAGESIZE - 1U) /**< \brief Page-offset mask */

/** Top of RAM currently made available to the KOS page allocator.

    This is a variable rather than a fixed architectural constant so later
    startup code can distinguish hosted XBE/loader execution from bare-metal
    startup and can detect expanded-memory systems. */
extern uintptr_t _arch_mem_top;

/** Bounds of the executable text used for validated stack unwinding. */
extern char _executable_start;
extern char _etext;

/** First page after the linked image and its reserved bootstrap stack. */
#define page_phys_base \
    (((uintptr_t)end + (uintptr_t)PAGEMASK) & ~(uintptr_t)PAGEMASK)

/** Number of complete pages exposed to the allocator. */
#define page_count \
    ((_arch_mem_top > page_phys_base) \
        ? ((_arch_mem_top - page_phys_base) / PAGESIZE) : 0U)

#ifndef THD_SCHED_HZ
#define THD_SCHED_HZ 100
#endif

static const unsigned HZ
    __depr("Please use the new THD_SCHED_HZ macro.") = THD_SCHED_HZ;

/* The Xbox toolchain uses leading underscores for C symbols in its PE/COFF
   objects. The loader's ELF conversion preserves those names. */
#define ELF_SYM_PREFIX      "_"
#define ELF_SYM_PREFIX_LEN  1

#define ARCH_NAME      "Xbox"
#define ARCH_ELFCLASS  ELFCLASS32
#define ARCH_ELFDATA   ELFDATA2LSB
#define ARCH_CODE      EM_386

/** Total RAM address space currently exposed to KOS. */
#define HW_MEMSIZE (_arch_mem_top)

void arch_panic(const char *str) __noreturn;
void arch_main(void) __noreturn;

#define ARCH_EXIT_RETURN 1
#define ARCH_EXIT_MENU   2
#define ARCH_EXIT_REBOOT 3

void arch_set_exit_path(int path);
void arch_exit(void) __noreturn;
void arch_return(int ret_code) __noreturn;
void arch_abort(void) __noreturn;
void arch_reboot(void) __noreturn;
void arch_menu(void) __noreturn;

/** Atomically enable interrupts and halt until an interrupt arrives.

    IA-32 defers recognition of maskable interrupts until after the instruction
    following STI, making `sti; hlt` the standard race-free idle sequence. */
static inline void arch_sleep(void) {
    __asm__ volatile("sti\n\t"
                     "hlt"
                     :
                     :
                     : "memory", "cc");
}

/** @} */

/* Preserve compatibility with code that obtains init flags through arch.h. */
#include <kos/init.h>

__END_DECLS

#endif /* __ARCH_ARCH_H */
