/* KallistiOS 2.0.0

   arch/gba/arch.h
   (c)2001 Dan Potter

*/

#ifndef __ARCH_ARCH_H
#define __ARCH_ARCH_H

#include <kos/cdefs.h>
#include <arch/types.h>

/* Number of timer ticks per second (if using threads) */
#define HZ      100

#define PAGESIZE    4096

#define _arch_mem_top 0x02040000

/* Default thread stack size (if using threads) */
#define THD_STACK_SIZE  8192

/* Do we need symbol prefixes? */
#define ELF_SYM_PREFIX "_"
#define ELF_SYM_PREFIX_LEN 1
/* Default video mode */

/* Default serial parameters */

/* Panic function */
void panic(char *str);

/* Prototype for the portable kernel main() */
int kernel_main(const char *args);

/* Kernel C-level entry point */
int arch_main();

void arch_panic(const char *str);

/** \brief  Generic kernel "exit" point.
    \note                   This function will never return!
*/
void arch_exit(void) __noreturn;

/** \brief  Kernel "return" point.
    \note                   This function will never return!
*/
void arch_return(int ret_code) __noreturn;

/** \brief  Kernel "abort" point.
    \note                   This function will never return!
*/
void arch_abort(void) __noreturn;

/** \brief  Kernel "reboot" call.
    \note                   This function will never return!
*/
void arch_reboot(void) __noreturn;

/* These are in mm.c */
/** \brief  Initialize the memory management system.
    \retval 0               On success (no error conditions defined).
*/
int mm_init();

/** \brief  Request more core memory from the system.
    \param  increment       The number of bytes requested.
    \return                 A pointer to the memory.
    \note                   This function will panic if no memory is available.
*/
void * mm_sbrk(unsigned long increment);

/* Use this macro to determine the level of initialization you'd like in
   your program by default. The defaults line will be fine for most things. */
#define KOS_INIT_FLAGS(flags)   uint32 __kos_init_flags = (flags)

extern uint32 __kos_init_flags;

/* Defaults */
#define INIT_DEFAULT \
    INIT_IRQ

/* Define a romdisk for your program, if you'd like one */
#define KOS_INIT_ROMDISK(rd)    void * __kos_romdisk = (rd)

extern void * __kos_romdisk;

/* State that you don't want a romdisk */
#define KOS_INIT_ROMDISK_NONE   NULL

/* Constants for the above */
#define INIT_NONE       0       /* Kernel enables */
#define INIT_IRQ        1
#define INIT_MALLOCSTATS    8

/* CPU sleep */
#define arch_sleep()


void arch_exec_at(const void *image, uint32 length, uint32 address) __noreturn;
void arch_exec(const void *image, uint32 length) __noreturn;

void arch_stk_trace(int n);
void arch_stk_trace_at(uint32 fp, int n);

void icache_flush_range(uint32 start, uint32 count);
void dcache_inval_range(uint32 start, uint32 count);
void dcache_flush_range(uint32 start, uint32 count);
void dcache_purge_range(uint32 start, uint32 count);
void *dcache_pref_range(uint32 start, uint32 count);

#endif  /* __ARCH_ARCH_H */

