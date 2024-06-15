/* KallistiOS ##version##

   arch/dreamcast/include/arch.h
   Copyright (C) 2001 Megan Potter
   Copyright (C) 2013, 2020 Lawrence Sebald

*/

/** \file    arch/arch.h
    \brief   Dreamcast architecture specific options.
    \ingroup arch

    This file has various architecture specific options defined in it. Also, any
    functions that start with arch_ are in here.

    \author Megan Potter
*/

#ifndef __ARCH_ARCH_H
#define __ARCH_ARCH_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/** \defgroup arch  Architecture
    \brief          Dreamcast Architecture-Specific Options and high-level API
    \ingroup        system
    @{
*/

/** \brief Scheduler interrupt frequency

    Timer interrupt frequency for the KOS thread scheduler.

    \note
    This value is what KOS uses initially upon startup, but it can be
    reconfigured at run-time.

    \sa thd_get_hz(), thd_set_hz()
*/
#define HZ              100

/** \brief  Default thread stack size. */
#define THD_STACK_SIZE  32768

/** \brief  Default video mode. */
#define DEFAULT_VID_MODE    DM_640x480

/** \brief  Default pixel mode for video. */
#define DEFAULT_PIXEL_MODE  PM_RGB565

/** \brief  Default serial bitrate. */
#define DEFAULT_SERIAL_BAUD 115200

/** \brief  Default serial FIFO behavior. */
#define DEFAULT_SERIAL_FIFO 1

/** \brief  Global symbol prefix in ELF files. */
#define ELF_SYM_PREFIX      "_"

/** \brief  Length of global symbol prefix in ELF files. */
#define ELF_SYM_PREFIX_LEN  1

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

/** \defgroup arch_retpaths Exit Paths
    \brief                  Potential exit paths from the kernel on
                            arch_exit()
    @{
*/
#define ARCH_EXIT_RETURN    1   /**< \brief Return to loader */
#define ARCH_EXIT_MENU      2   /**< \brief Return to system menu */
#define ARCH_EXIT_REBOOT    3   /**< \brief Reboot the machine */
/** @} */

/** \brief   Set the exit path.

    The default, if you don't call this, is ARCH_EXIT_RETURN.

    \param  path            What arch_exit() should do.
    \see    arch_retpaths
*/
void arch_set_exit_path(int path);

/** \brief   Generic kernel "exit" point.

    \note                   This function will never return!
*/
void arch_exit(void) __noreturn;

/** \brief   Kernel "return" point.

    \note                   This function will never return!
*/
void arch_return(int ret_code) __noreturn;

/** \brief   Kernel "abort" point.

    \note                   This function will never return!
*/
void arch_abort(void) __noreturn;

/** \brief   Kernel "reboot" call.

    \note                   This function will never return!
*/
void arch_reboot(void) __noreturn;

/** \brief   Kernel "exit to menu" call.

    \note                   This function will never return!
*/
void arch_menu(void) __noreturn;

/** @} */

/* Pull in the memory management API, which used to be here, for 
   compatibility. */
#include <arch/mm.h>

/* Bring in the init flags for compatibility with old code that expects them
   here. */
#include <kos/init.h>

/** \addtogroup arch
    @{
*/

/* Dreamcast-specific arch init things */
/** \brief   Jump back to the bootloader.

    You generally shouldn't use this function, but rather use arch_exit() or
    exit() instead.

    \note                   This function will never return!
*/
void arch_real_exit(int ret_code) __noreturn;

/** \brief   Initialize bare-bones hardware systems.

    This will be done automatically for you on start by the default arch_main(),
    so you shouldn't have to deal with this yourself.

    \retval 0               On success (no error conditions defined).
*/
int hardware_sys_init(void);

/** \brief   Initialize some peripheral systems.

    This will be done automatically for you on start by the default arch_main(),
    so you shouldn't have to deal with this yourself.

    \retval 0               On success (no error conditions defined).
*/
int hardware_periph_init(void);

/** \brief   Shut down hardware that was initted.

    This function will shut down anything initted with hardware_sys_init() and
    hardware_periph_init(). This will be done for you automatically by the
    various exit points, so you shouldn't have to do this yourself.
*/
void hardware_shutdown(void);

/** \defgroup hw_consoles           Console Types
    \brief                          Byte values returned by hardware_sys_mode()

    These are the various console types that can be returned by the
    hardware_sys_mode() function.

    @{
*/
#define HW_TYPE_RETAIL      0x0     /**< \brief A retail Dreamcast. */
#define HW_TYPE_SET5        0x9     /**< \brief A Set5.xx devkit. */
/** @} */

/** \defgroup hw_regions            Region Codes
    \brief                          Values returned by hardware_sys_mode();

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

/** \brief   Dreamcast specific sleep mode "function". */
#define arch_sleep() do { \
        __asm__ __volatile__("sleep"); \
    } while(0)

/** \brief   DC specific "function" to get the return address from the current
             function.

    \return                 The return address of the current function.
*/
#define arch_get_ret_addr() ({ \
        uint32 pr; \
        __asm__ __volatile__("sts	pr,%0\n" \
                             : "=&z" (pr) \
                             : /* no inputs */ \
                             : "memory" ); \
        pr; })

/* Please note that all of the following frame pointer macros are ONLY
   valid if you have compiled your code WITHOUT -fomit-frame-pointer. These
   are mainly useful for getting a stack trace from an error. */

/** \brief   DC specific "function" to get the frame pointer from the current
             function.

    \return                 The frame pointer from the current function.
    \note                   This only works if you don't disable frame pointers.
*/
#define arch_get_fptr() ({ \
        uint32 fp; \
        __asm__ __volatile__("mov	r14,%0\n" \
                             : "=&z" (fp) \
                             : /* no inputs */ \
                             : "memory" ); \
        fp; })

/** \brief   Pass in a frame pointer value to get the return address for the
             given frame.

    \param  fptr            The frame pointer to look at.
    \return                 The return address of the pointer.
*/
#define arch_fptr_ret_addr(fptr) (*((uint32*)(fptr)))

/** \brief   Pass in a frame pointer value to get the previous frame pointer for
             the given frame.

    \param  fptr            The frame pointer to look at.
    \return                 The previous frame pointer.
*/
#define arch_fptr_next(fptr) (*((uint32*)((fptr)+4)))

/** \brief   Returns true if the passed address is likely to be valid. Doesn't
             have to be exact, just a sort of general idea.

    \return                 Whether the address is valid or not for normal
                            memory access.
*/
#define arch_valid_address(ptr) ((ptr_t)(ptr) >= 0x8c010000 && (ptr_t)(ptr) < _arch_mem_top)

/** @} */

/* These three aught to be in their own header file at some point, but for now,
   they'll stay here. */

/** \addtogroup attribution
    @{
*/

/** \brief   Retrieve the banner printed at program initialization.

    This function retrieves the banner string that is printed at initialization
    time by the kernel. This contains the version of KOS in use and basic
    information about the environment in which it was compiled.

    \return                 A pointer to the banner string.
*/
const char *kos_get_banner(void);

/** \brief   Retrieve the license information for the compiled copy of KOS.

    This function retrieves a string containing the license terms that the
    version of KOS in use is distributed under. This can be used to easily add
    information to your program to be displayed at runtime.

    \return                 A pointer to the license terms.
*/
const char *kos_get_license(void);

/** \brief   Retrieve a list of authors and the dates of their contributions.

    This function retrieves the copyright information for the version of KOS in
    use. This function can be used to add such information to the credits of
    programs using KOS to give the appropriate credit to those that have worked
    on KOS.

    \remark
    Remember, you do need to give credit where credit is due, and this is an
    easy way to do so. ;-)

    \return                 A pointer to the authors' copyright information.
*/
const char *kos_get_authors(void);

/** @} */

__END_DECLS

#endif  /* __ARCH_ARCH_H */
