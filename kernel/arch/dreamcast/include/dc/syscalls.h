/* KallistiOS ##version##

   dc/syscalls.h
   Copyright (C) 2024 Andy Barajas
*/

/** \file      dc/syscalls.h
    \brief     Functions to access the system calls of the Dreamcast ROM.
    \ingroup   system_calls

\todo
    - syscall_sysinfo_icon(): Discover + document icon format.
    - Look into additional syscall vector for GD-ROM - 0x0C0000C0

    \author Marcus Comstedt
    \author Andy Barajas
*/

/** \defgroup  system_calls System Calls
    \brief     API for the Dreamcast's system calls
    \ingroup   system

    This module encapsulates all the system calls available in the Dreamcast 
    BIOS, allowing direct interaction with system hardware 
    components such as the GDROM drive, flash ROM, and bios fonts. These 
    functions are essential for performing low-level operations that are not
    handled by standard user-space APIs.
    
    @{
*/

#ifndef __DC_SYSCALLS_H
#define __DC_SYSCALLS_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <sys/types.h>

/** \brief   Inits data needed by sysinfo id/icon
    \note This is called automatically by KOS during initialization.
    This function prepares syscall_sysinfo_icon and syscall_sysinfo_id 
    calls for use by copying the relevant data from the system flashrom 
    into 8C000068-8C00007F.
*/
void syscall_sysinfo_init(void);

/** \brief   Reads an icon from the flashrom.

    This function reads an icon from the flashrom into a destination 
    buffer.

    \note
    The format of these icons is not known.

    \param  icon            The icon number (0-9, 5-9 seems to really 
                            be icons).
    \param  dest            The destination buffer (704 bytes in size).

    \return                 Number of bytes read on success, or -1 on
                            failure.
*/
int syscall_sysinfo_icon(uint32_t icon, uint8_t *dest);

/** \brief   Reads the ID of the Dreamcast.

    This function returns the unique 64-bit ID for the Dreamcast.

    \return                 The Dreamcast ID.
*/
uint64_t syscall_sysinfo_id(void);

/** \brief   Gets the romfont address.

    This function returns the address of the ROM font.

    \warning
    Before attempting to access the font data, you should always call
    syscall_font_lock() to ensure that you have exclusive access to the
    G1 BUS the ROM is located on. Call syscall_font_unlock() when you're
    done accessing the data.

    \note
    Defined in syscall_font.s

    \return                 The address of the font.
*/
uint8_t *syscall_font_address(void);

/** \brief   Locks access to ROM font.

    This function tries to lock a mutex for exclusive access to the ROM 
    font. This is needed because you can't access the BIOS font during 
    G1 DMA.

    \note
    Defined in syscall_font.s

    \retval 0               On success.
    \retval -1              On failure.

    \sa syscall_font_unlock()
*/
int syscall_font_lock(void);

/** \brief   Unlocks access to ROM font.
    \ingroup system_calls

    This function releases the mutex locked with syscall_font_lock().

    \note
    Defined in syscall_font.s

    \sa syscall_font_lock()
*/
void syscall_font_unlock(void);

/** \brief   Gets info on partition in the flashrom.

    This function fetches the info of a partition in the flashrom.

    \param  part            The partition number (0-4).
    \param  info            The buffer to store info (8 bytes in size).

    \retval 0               On success.
    \retval -1              On failure.
*/
int syscall_flashrom_info(uint32_t part, void *info);

/** \brief   Read data from the flashrom.

    This function reads data from an offset into the flashrom to the 
    destination buffer.

    \param  pos             The read start position into the flashrom.
    \param  dest            The destination buffer.
    \param  n               The number of bytes to read.

    \return                 Number of bytes read on success, or -1 on
                            failure.

    \sa syscall_flashrom_write(), syscall_flashrom_delete()
*/
int syscall_flashrom_read(uint32_t pos, void *dest, size_t n);

/** \brief   Write data to the flashrom.

    This function writes data to an offset into the flashrom from the 
    source buffer.

    \warning
    It is only possible to overwrite 1's with 0's. 0's can not be written 
    back to 1's so general overwriting is therefore not possible.  You 
    would need to delete a whole partition to overwrite it.

    \param  pos             The start position to write into the flashrom.
    \param  src             The source buffer.
    \param  n               The number of bytes to write.

    \return                 Number of bytes written on success, or -1 on
                            failure.

    \sa syscall_flashrom_read(), syscall_flashrom_delete()
*/
int syscall_flashrom_write(uint32_t pos, const void *src, size_t n);

/** \brief   Delete a partition of the flashrom.

    This function returns a flashrom partition to all 1's, so that it may 
    be rewritten.

    \warning
    ALL data in the entire partition will be lost.

    \param  pos             The offset from the start of the flashrom you 
                            want to delete.

    \retval 0               On success.
    \retval -1              On failure.

    \sa syscall_flashrom_read(), syscall_flashrom_write()
*/
int syscall_flashrom_delete(uint32_t pos);

/** \brief   Initialize the GDROM drive.

    This function initializes the GDROM drive. Should be called before any 
    commands are sent.
*/
void syscall_gdrom_init(void);

/** \brief   Reset the GDROM drive.

    This function resets the GDROM drive.
*/
void syscall_gdrom_reset(void);

/** \brief   Checks the GDROM drive status.

    This function retrieves the general condition of the GDROM drive. It 
    populates a provided array with two elements. The first element 
    indicates the current drive status, and the second element identifies 
    the type of disk inserted (if any).

    \param  status          The pointer to two 32-bit unsigned integers to 
                            receive extended status information.

    \return                 0 on success, or non-zero on
                            failure.
*/
int syscall_gdrom_check_drive(uint32_t status[2]);

/** \brief   Send a command to the GDROM command queue.

    This function sends a command to the GDROM queue.

    \note
    Call syscall_gdrom_exec_server() to run queued commands.

    \param  cmd             The command code (see CMD_* in \ref dc/cdrom.h).
    \param  params          The pointer to parameter block for the command, 
                            can be NULL if the command does not take 
                            parameters.

    \return                 The request id (>=1) on success, or 0 on failure.

    \sa syscall_gdrom_check_command(), syscall_gdrom_exec_server()
*/
uint32_t syscall_gdrom_send_command(uint32_t cmd, void *params);

/** \brief   Check status of queued command for the GDROM.

    This function checks if a queued command has completed.

    \param  id              The request id (>=1).
    \param  status          The pointer to four 32-bit integers to 
                            receive status information.

    \retval -1              Request has failed.
    \retval 0               Request not found.
    \retval 1               Request is still being processed.
    \retval 2               Request completed successfully.
    \retval 3               Stream type command is in progress.
    \retval 4               GD syscalls are busy.

    \sa syscall_gdrom_send_command(), syscall_gdrom_exec_server()
*/
int syscall_gdrom_check_command(uint32_t id, int32_t status[4]);

/** \brief   Process queued GDROM commands.

    This function starts processing queued commands. This must be 
    called a few times to process all commands. An example of it in 
    use can be seen in \sa cdrom_exec_cmd_timed() (see hardware/cdrom.c).

    \sa syscall_gdrom_send_command(), syscall_gdrom_check_command()
*/
void syscall_gdrom_exec_server(void);

/** \brief   Abort a queued GDROM command.

    This function tries to abort a previously queued command.

    \param  id              The request id (>=1) to abort.

    \return                 0 on success, or non-zero on
                            failure.
*/
int syscall_gdrom_abort_command(uint32_t id);

/** \brief   Sets/gets the sector mode for read commands.

    This function sets/gets the sector mode for read commands.

    \param  mode            The pointer to a struct of four 32 bit integers 
                            containing new values, or to receive the old 
                            values.

    \retval 0               On success.
    \retval -1              On failure.
*/
int syscall_gdrom_sector_mode(uint32_t mode[4]);

/** \brief   Setup GDROM DMA callback.

    This function sets up DMA transfer end callback for 
    \ref CMD_DMAREAD_STREAM_EX (\ref dc/cdrom.h).

    \param  callback        The function to call upon completion of the DM.
    \param  param           The data to pass to the callback function.
*/
void syscall_gdrom_dma_callback(uintptr_t callback, void *param);

/** \brief   Initiates a GDROM DMA transfer.

    This function initiates a DMA transfer for 
    \ref CMD_DMAREAD_STREAM_EX (\ref dc/cdrom.h).

    \param  id              The request id (>=1).
    \param  params          The pointer to two 32-bit integers. The first 
                            element indicates the destination address, and 
                            the second element identifies how many bytes to 
                            transfer.

    \return                 0 on success, or non-zero on
                            failure.
*/
int syscall_gdrom_dma_transfer(uint32_t id, const int32_t params[2]);

/** \brief   Checks a GDROM DMA transfer.

    This function checks the progress of a DMA transfer for 
    \ref CMD_DMAREAD_STREAM_EX (see \ref dc/cdrom.h).

    \param  id              The request id (>=1).
    \param  size            The pointer to receive the remaining amount of
                            bytes to transfer.

    \retval 0               On success.
    \retval -1              On failure.
*/
int syscall_gdrom_dma_check(uint32_t id, size_t *size);

/** \brief   Setup GDROM PIO callback.

    This function sets up PIO transfer end callback for 
    \ref CMD_PIOREAD_STREAM_EX (see \ref dc/cdrom.h).

    \param  callback        The function to call upon completion of the
                            transfer.
    \param  param           The data to pass to the callback function.
*/
void syscall_gdrom_pio_callback(uintptr_t callback, void *param);

/** \brief   Initiates a GDROM PIO transfer.

    This function initiates a PIO transfer for 
    \ref CMD_PIOREAD_STREAM_EX (see \ref dc/cdrom.h).

    \param  id              The request id (>=1).
    \param  params          The pointer to two 32-bit integers. The first 
                            element indicates the destination address, and 
                            the second element identifies how many bytes to 
                            transfer.

    \return                 0 on success, or non-zero on
                            failure.
*/
int syscall_gdrom_pio_transfer(uint32_t id, const int32_t params[2]);

/** \brief   Checks a GDROM PIO transfer.

    This function checks the progress of a PIO transfer for 
    \ref CMD_PIOREAD_STREAM_EX (see \ref dc/cdrom.h).

    \param  id              The request id (>=1).
    \param  size            The pointer to receive the remaining amount of
                            bytes to transfer.

    \retval 0               On success.
    \retval -1              On failure.
*/
int syscall_gdrom_pio_check(uint32_t id, size_t *size);

/** \brief   Initializes all the syscall vectors.

    This function initializes all the syscall vectors to their default values.

    \return                 0
*/
int syscall_misc_init(void);

/** \brief   Set/Clear a user defined super function.

    This function sets/clears the handler for one of the seven user defined
    super functions. Setting a handler is only allowed if it not currently set.

    \param  super           The super function number (1-7).
    \param  handler         The pointer to handler function, or NULL to
                            clear.

    \retval 0               On success.
    \retval -1              On failure.
*/
int syscall_misc_setvector(uint32_t super, uintptr_t handler);

/** \brief   Resets the Dreamcast.

    This function soft resets the Dreamcast console.
*/
void syscall_system_reset(void) __noreturn;

/** \brief   Go to the BIOS menu.

    This function exits the program to the BIOS menu.
*/
void syscall_system_bios_menu(void) __noreturn;

/** \brief   Exit to CD menu.

    This function exits the program to the BIOS CD menu.
*/
void syscall_system_cd_menu(void) __noreturn;

/** @} */ 

__END_DECLS

#endif

