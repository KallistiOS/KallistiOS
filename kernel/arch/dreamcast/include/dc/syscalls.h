/* KallistiOS ##version##

   dc/syscalls.h
   Copyright (C) 2024 Andy Barajas
*/

/** \file      dc/syscalls.h
    \brief     Functions to access the system calls of the Dreamcast ROM.
    \ingroup   system_calls

    \author Marcus Comstedt
    \author Andy Barajas
*/

/** \defgroup  system_calls System Calls
    \brief     API for the Dreamcast's system calls
    \ingroup   system

    This module encapsulates all the system calls available in the Dreamcast 
    Operating System, allowing direct interaction with system hardware 
    components such as the GDROM drive, flash ROM, and bios fonts. These 
    functions are essential for performing low-level operations that are not
    handled by standard user-space APIs.
*/

#ifndef __DC_SYSCALLS_H
#define __DC_SYSCALLS_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <sys/types.h>

/** \brief   Reads an icon from the flashrom.
    \ingroup system_calls

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
    \ingroup system_calls

    This function returns the unique 64-bit ID for the Dreamcast.

    \return                 The Dreamcast ID.
*/
uint64_t syscall_sysinfo_id(void);

/** \brief   Gets the romfont address.
    \ingroup system_calls

    This function returns the address of the ROM font.

    \note
    Defined in syscall_font.s

    \return                 The address of the font.
*/
uint8_t *syscall_font_address(void);

/** \brief   Locks access to ROM font.
    \ingroup system_calls

    This function tries to lock a mutex for exclusive access to the ROM 
    font. This is needed because you can't access the BIOS font during 
    disk DMA.

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
    \ingroup system_calls

    This function fetches the info of a partition in the flashrom.

    \param  part            The partition number (0-4).
    \param  info            The buffer to store info (8 bytes in size).

    \retval 0               On success.
    \retval -1              On failure.
*/
int syscall_flashrom_info(uint32_t part, void *info);

/** \brief   Read data from the flashrom.
    \ingroup system_calls

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
    \ingroup system_calls

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
    \ingroup system_calls

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
    \ingroup system_calls

    This function initializes the GDROM drive. Should be called before any 
    commands are sent.
*/
void syscall_gdrom_init(void);

/** \brief   Reset the GDROM drive.
    \ingroup system_calls

    This function resets the GDROM drive.
*/
void syscall_gdrom_reset(void);

/** \brief   Checks the GDROM drive status.
    \ingroup system_calls

    This function retrieves the general condition of the GDROM drive. It 
    populates a provided array with two elements. The first element 
    indicates the current drive status, and the second element identifies 
    the type of disk inserted (if any).

    \param  status          The pointer to two 32-bit integers to 
                            receive extended status information.

    \return                 0 on success, or non-zero on
                            failure.
*/
int syscall_gdrom_check_drive(void *status);

/** \brief   Send a command to the GDROM command queue.
    \ingroup system_calls

    This function sends a command to the GDROM queue.

    \note
    Call syscall_gdrom_run_commands() to run queued commands.

    \param  cmd             The command code (see CMD_* in cdrom.h).
    \param  params          The pointer to parameter block for the command, 
                            can be NULL if the command does not take 
                            parameters.

    \return                 The request id (>=0) on success, or <0 error
                            code on failure.

    \sa syscall_gdrom_check_command(), syscall_gdrom_run_commands()
*/
int syscall_gdrom_send_command(uint8_t cmd, void *params);

/** \brief   Check status of queued command for the GDROM.
    \ingroup system_calls

    This function checks if a queued command has completed.

    \param  id              The request id (>=0).
    \param  status          The pointer to four 32-bit integers to 
                            receive status information.

    \retval 0               Request not found.
    \retval 1               Request is still being processed.
    \retval 2               Request completed successfully.
    \retval 3               Stream type command is in progress.
    \retval 4               GD syscalls are busy.

    \sa syscall_gdrom_send_command(), syscall_gdrom_run_commands()
*/
int syscall_gdrom_check_command(uint8_t id, void *status);

/** \brief   Process queued GDROM commands.
    \ingroup system_calls

    This function starts processing queued commands. This must be 
    called a few times to process all commands.

    \sa syscall_gdrom_send_command(), syscall_gdrom_check_command()
*/
void syscall_gdrom_run_commands(void);

/** \brief   Abort a queued GDROM command.
    \ingroup system_calls

    This function tries to abort a previously queued command.

    \param  id              The request id (>=0) to abort.

    \return                 0 on success, or non-zero on
                            failure.
*/
int syscall_gdrom_abort_command(uint8_t id);

/** \brief   Sets/gets the sector mode for read commands.
    \ingroup system_calls

    This function sets/gets the sector mode for read commands.

    \param  status          The pointer to a struct of four 32 bit integers 
                            containing new values, or to receive the old 
                            values.

    \retval 0               On success.
    \retval -1              On failure.
*/
int syscall_gdrom_sector_mode(void *mode);

/** \brief   Clear user defined vectors.
    \ingroup system_calls

    This function clears all the user defined syscall vectors.

    \return                 0
*/
int syscall_misc_init(void);

/** \brief   Register/Clear a user defined vector
    \ingroup system_calls

    This function sets/clears the handler for one of the seven user defined
    vectors. Setting a handler is only allowed if it not currently set.

    \param  vector          The vector number (1-7).
    \param  handler         The pointer to handler function, or NULL to
                            clear.

    \retval 0               On success.
    \retval -1              On failure.
*/
int syscall_misc_setvector(uint8_t super, uintptr_t handler);

/** \brief   Setup GDROM DMA callback
    \ingroup system_calls

    This function sets up DMA transfer end callback for 
    CMD_DMAREAD_STREAM_EX (see cdrom.h).

    \param  callback        The function to call upon completion of the DM.
    \param  param           The data to pass to the callback function.
*/
void syscall_dma_callback(uintptr_t callback, void *param);

/** \brief   Initiates a DMA transfer
    \ingroup system_calls

    This function initiates a DMA transfer for 
    CMD_DMAREAD_STREAM_EX (see cdrom.h).

    \param  id              The request id (>=0).
    \param  param           The pointer to two 32-bit integers. The first 
                            element indicates the destination address, and 
                            the second element identifies how many bytes to 
                            transfer.

    \return                 0 on success, or non-zero on
                            failure.
*/
int syscall_dma_transfer(int id, int *params);

/** \brief   Checks a DMA transfer
    \ingroup system_calls

    This function checks the progress of a DMA transfer for 
    CMD_DMAREAD_STREAM_EX (see cdrom.h).

    \param  id              The request id (>=0).
    \param  size            The pointer to receive the remaining amount of
                            bytes to transfer.

    \retval 0               On success.
    \retval -1              On failure.
*/
int syscall_dma_check(int id, int *size);

/** \brief   Setup GDROM PIO callback
    \ingroup system_calls

    This function sets up PIO transfer end callback for 
    CMD_PIOREAD_STREAM_EX (see cdrom.h).

    \param  callback        The function to call upon completion of the
                            transfer.
    \param  param           The data to pass to the callback function.
*/
void syscall_pio_callback(uintptr_t callback, void *param);

/** \brief   Initiates a PIO transfer
    \ingroup system_calls

    This function initiates a PIO transfer for 
    CMD_PIOREAD_STREAM_EX (see cdrom.h).

    \param  id              The request id (>=0).
    \param  param           The pointer to two 32-bit integers. The first 
                            element indicates the destination address, and 
                            the second element identifies how many bytes to 
                            transfer.

    \return                 0 on success, or non-zero on
                            failure.
*/
int syscall_pio_transfer(int id, int *params);

/** \brief   Checks a PIO transfer
    \ingroup system_calls

    This function checks the progress of a PIO transfer for 
    CMD_PIOREAD_STREAM_EX (see cdrom.h).

    \param  id              The request id (>=0).
    \param  size            The pointer to receive the remaining amount of
                            bytes to transfer.

    \retval 0               On success.
    \retval -1              On failure.
*/
int syscall_pio_check(int id, int *size);

__END_DECLS

#endif