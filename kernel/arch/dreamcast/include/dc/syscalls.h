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

/** \defgroup   gdrom_syscalls
    \brief      GDROM Syscalls and Data Types
    \ingroup    system_calls
    \ingroup    gdrom

    These are the syscalls that allow operation of the GDROM drive
    as well as data types for their returns and parameters.
*/

/** \brief      Status of GDROM drive
    \ingroup    gdrom_syscalls

    These are the values that can be returned as the first param of
    syscall_gdrom_check_drive.
*/
typedef enum cd_stat {
    CD_STATUS_READ_FAIL = -1,   /**< \brief Can't read status */
    CD_STATUS_BUSY      =  0,   /**< \brief Drive is busy */
    CD_STATUS_PAUSED    =  1,   /**< \brief Disc is paused */
    CD_STATUS_STANDBY   =  2,   /**< \brief Drive is in standby */
    CD_STATUS_PLAYING   =  3,   /**< \brief Drive is currently playing */
    CD_STATUS_SEEKING   =  4,   /**< \brief Drive is currently seeking */
    CD_STATUS_SCANNING  =  5,   /**< \brief Drive is scanning */
    CD_STATUS_OPEN      =  6,   /**< \brief Disc tray is open */
    CD_STATUS_NO_DISC   =  7,   /**< \brief No disc inserted */
    CD_STATUS_RETRY     =  8,   /**< \brief Retry is needed */
    CD_STATUS_ERROR     =  9,   /**< \brief System error */
    CD_STATUS_FATAL     =  12,  /**< \brief Need reset syscalls */
} cd_stat_t;

/** \brief      Disc types the GDROM can identify
    \ingroup    gdrom_syscalls

    These are the values that can be returned as the second param of
    syscall_gdrom_check_drive.
*/
typedef enum cd_disc_types {
    CD_CDDA     = 0x00,    /**< \brief Audio CD (Red book) or no disc */
    CD_CDROM    = 0x10,    /**< \brief CD-ROM or CD-R (Yellow book) */
    CD_CDROM_XA = 0x20,    /**< \brief CD-ROM XA (Yellow book extension) */
    CD_CDI      = 0x30,    /**< \brief CD-i (Green book) */
    CD_GDROM    = 0x80,    /**< \brief GD-ROM */
    CD_FAIL     = 0xf0     /**< \brief Need reset syscalls */
} cd_disc_types_t;

/** \brief      Status filled by Check Drive syscall
    \ingroup    gdrom_syscalls
*/
typedef struct cd_check_drive_status {
    cd_stat_t       status;
    cd_disc_types_t disc_type;
} cd_check_drive_status_t;

/** \brief      Handle for a requested command
    \ingroup    gdrom_syscalls

    This is returned by syscall_gdrom_send_command and then is passed
    to other syscalls to specify which command to act on.
*/
typedef int32_t gdc_cmd_hnd_t;

/** \brief      Command codes for GDROM syscalls
    \ingroup    gdrom_syscalls

    These are the syscall command codes used to actually do stuff with the
    GDROM drive. These were originally provided by maiwe.
*/
typedef enum cd_cmd_code {
    CD_CMD_CHECK_LICENSE     =  2,  /**< \brief Check license */
    CD_CMD_REQ_SPI_CMD       =  4,  /**< \brief Request to Sega Packet Interface */
    CD_CMD_PIOREAD           = 16,  /**< \brief Read via PIO */
    CD_CMD_DMAREAD           = 17,  /**< \brief Read via DMA */
    CD_CMD_GETTOC            = 18,  /**< \brief Read TOC */
    CD_CMD_GETTOC2           = 19,  /**< \brief Read TOC */
    CD_CMD_PLAY_TRACKS       = 20,  /**< \brief Play track */
    CD_CMD_PLAY_SECTORS      = 21,  /**< \brief Play sectors */
    CD_CMD_PAUSE             = 22,  /**< \brief Pause playback */
    CD_CMD_RELEASE           = 23,  /**< \brief Resume from pause */
    CD_CMD_INIT              = 24,  /**< \brief Initialize the drive */
    CD_CMD_DMA_ABORT         = 25,  /**< \brief Abort DMA transfer */
    CD_CMD_OPEN_TRAY         = 26,  /**< \brief Open CD tray (on DevBox?) */
    CD_CMD_SEEK              = 27,  /**< \brief Seek to a new position */
    CD_CMD_DMAREAD_STREAM    = 28,  /**< \brief Stream DMA until end/abort */
    CD_CMD_NOP               = 29,  /**< \brief No operation */
    CD_CMD_REQ_MODE          = 30,  /**< \brief Request mode */
    CD_CMD_SET_MODE          = 31,  /**< \brief Setup mode */
    CD_CMD_SCAN_CD           = 32,  /**< \brief Scan CD */
    CD_CMD_STOP              = 33,  /**< \brief Stop the disc from spinning */
    CD_CMD_GETSCD            = 34,  /**< \brief Get subcode data */
    CD_CMD_GETSES            = 35,  /**< \brief Get session */
    CD_CMD_REQ_STAT          = 36,  /**< \brief Request stat */
    CD_CMD_PIOREAD_STREAM    = 37,  /**< \brief Stream PIO until end/abort */
    CD_CMD_DMAREAD_STREAM_EX = 38,  /**< \brief Stream DMA transfer */
    CD_CMD_PIOREAD_STREAM_EX = 39,  /**< \brief Stream PIO transfer */
    CD_CMD_GET_VERS          = 40,  /**< \brief Get syscall driver version */
    CD_CMD_MAX               = 47,  /**< \brief Max of GD syscall commands */
} cd_cmd_code_t;

/** \brief      Params for READ commands.
    \ingroup    gdrom_syscalls

    These are the parameters for the CMD_PIOREAD and CMD_DMAREAD commands.

*/
typedef struct cd_read_params {
    uint32_t    start_sec;  /* Starting sector */
    size_t      num_sec;    /* Number of sectors */
    void        *buffer;    /* Output buffer */
    uint32_t    is_test;    /* Enable test mode */
} cd_read_params_t;

/** \brief      Disc area to read TOC from.
    \ingroup    gdrom_syscalls

    This is the allowed values for the first param of the GETTOC commands,
    defining which disc area to read the TOC from.

*/
typedef enum cd_area {
    CD_AREA_LOW     = 0,
    CD_AREA_HIGH    = 1
} cd_area_t;

/** \brief      TOC structure returned by the BIOS.
    \ingroup    gdrom_syscalls

    This is the structure that the CD_CMD_GETTOC2 syscall command will return for
    the TOC. Note the data is in FAD, not LBA/LSN.

*/
typedef struct cd_toc {
    uint32_t  entry[99];          /**< \brief TOC space for 99 tracks */
    uint32_t  first;              /**< \brief Point A0 information (1st track) */
    uint32_t  last;               /**< \brief Point A1 information (last track) */
    uint32_t  leadout_sector;     /**< \brief Point A2 information (leadout) */
} cd_toc_t;

/** \brief      Params for GETTOC commands
    \ingroup    gdrom_syscalls

    Params for CD_CMD_GETTOC and CD_CMD_GETTOC2.

*/
typedef struct cd_cmd_toc_params {
    cd_area_t  area;
    cd_toc_t   *buffer;
} cd_cmd_toc_params_t;

/** \brief      Params for PLAY command
    \ingroup    gdrom_syscalls

    Params for CD_CMD_PLAY_TRACKS and CD_CMD_PLAY_SECTORS.

*/
typedef struct cd_cmd_play_params {
    uint32_t start;     /**< \brief Track to play from */
    uint32_t end;       /**< \brief Track to play to */
    uint32_t repeat;    /**< \brief Times to repeat (0-15, 15=infinite) */
} cd_cmd_play_params_t;

/** \brief      Types of data to read from sector subcode
    \ingroup    gdrom_syscalls

    Types of data available to read from the sector subcode. These are
    possible values for the first parameter sent to the GETSCD syscall.
*/
typedef enum cd_sub_type {
    CD_SUB_Q_ALL          = 0,    /**< \brief Read all Subcode Data */
    CD_SUB_Q_CHANNEL      = 1,    /**< \brief Read Q Channel Subcode Data */
    CD_SUB_MEDIA_CATALOG  = 2,    /**< \brief Read the Media Catalog Subcode Data */
    CD_SUB_TRACK_ISRC     = 3,    /**< \brief Read the ISRC Subcode Data */
    CD_SUB_RESERVED       = 4     /**< \brief Reserved */
} cd_sub_type_t;

/** \brief      Params for GETSCD command
    \ingroup    gdrom_syscalls
*/
typedef struct cd_cmd_getscd_params {
    cd_sub_type_t   which;    /**< \brief The type of subcode read to perform */
    size_t          buflen;   /**< \brief The size of the buffer we provide */
    void            *buffer;  /**< \brief The buffer to put the subcode data in */
} cd_cmd_getscd_params_t;

/** \brief      Subcode Audio Statuses
    \ingroup    gdrom_syscalls

    Information about CDDA playback returned by the GETSCD syscall command.
    This is returned in the second byte of the buffer.
*/
typedef enum cd_sub_audio {
    CD_SUB_AUDIO_STATUS_INVALID    = 0x00,
    CD_SUB_AUDIO_STATUS_PLAYING    = 0x11,
    CD_SUB_AUDIO_STATUS_PAUSED     = 0x12,
    CD_SUB_AUDIO_STATUS_ENDED      = 0x13,
    CD_SUB_AUDIO_STATUS_ERROR      = 0x14,
    CD_SUB_AUDIO_STATUS_NO_INFO    = 0x15
} cd_sub_audio_t;

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
    indicates the current drive status (cd_stat_t), and the second
    element identifies the type of disc inserted if any (cd_disc_types_t).

    \param  status          A cd_check_drive_status_t filled with drive
                            status information.

    \return                 0 on success, or non-zero on
                            failure.
*/
int syscall_gdrom_check_drive(cd_check_drive_status_t *status);

/** \brief   Send a command to the GDROM command queue.

    This function sends a command to the GDROM queue.

    \note
    Call syscall_gdrom_exec_server() to run queued commands.

    \param  cmd             The command code (see cd_cmd_code_t above).
    \param  params          The pointer to parameter block for the command, 
                            can be NULL if the command does not take 
                            parameters.

    \return                 The request id (>=1) on success, or 0 on failure.

    \sa syscall_gdrom_check_command(), syscall_gdrom_exec_server()
*/
gdc_cmd_hnd_t syscall_gdrom_send_command(cd_cmd_code_t cmd, void *params);

/** \brief   Check status of queued command for the GDROM.

    This function checks if a queued command has completed.

    \param  hnd             The request to check.
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
int syscall_gdrom_check_command(gdc_cmd_hnd_t hnd, int32_t status[4]);

/** \brief   Process queued GDROM commands.

    This function starts processing queued commands. This must be 
    called a few times to process all commands. An example of it in 
    use can be seen in \sa cdrom_exec_cmd_timed() (see hardware/cdrom.c).

    \sa syscall_gdrom_send_command(), syscall_gdrom_check_command()
*/
void syscall_gdrom_exec_server(void);

/** \brief   Abort a queued GDROM command.

    This function tries to abort a previously queued command.

    \param  hnd             The request to abort.

    \return                 0 on success, or non-zero on
                            failure.
*/
int syscall_gdrom_abort_command(gdc_cmd_hnd_t hnd);

/** \brief      Read Sector Part
    \ingroup    gdrom_syscalls

    Parts of the a disc sector to read. These are possible values for the
    second parameter word sent with syscall_gdrom_sector_mode.

    \note CD_READ_DEFAULT not supported by the syscall and is provided
    for compatibility in cdrom_reinit_ex
*/
typedef enum cd_read_sec_part {
    CDROM_READ_WHOLE_SECTOR = 0x1000,    /**< \brief Read the whole sector */
    CDROM_READ_DATA_AREA    = 0x2000,    /**< \brief Read the data area */
    CDROM_READ_DEFAULT      = -1         /**< \brief cdrom_reinit default */
} cd_read_sec_part_t;

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

/** \brief      Parameters for transfer
    \ingroup    gdrom_syscalls

    These are parameters passed to the syscall_gdrom_*_transfer syscalls.
*/
typedef struct cd_transfer_params {
    void    *addr;      /**< \brief Destination address of transfer */
    size_t  size;       /**< \brief How many bytes to transfer */
} cd_transfer_params_t;

/** \brief   Initiates a GDROM DMA transfer.

    This function initiates a DMA transfer for 
    \ref CMD_DMAREAD_STREAM_EX (\ref dc/cdrom.h).

    \param  hnd             The stream request to start transferring.
    \param  params          The pointer to a cd_transfer_params_t.

    \return                 0 on success, or non-zero on
                            failure.
*/
int syscall_gdrom_dma_transfer(gdc_cmd_hnd_t hnd, const cd_transfer_params_t *params);

/** \brief   Checks a GDROM DMA transfer.

    This function checks the progress of a DMA transfer for 
    \ref CMD_DMAREAD_STREAM_EX (see \ref dc/cdrom.h).

    \param  hnd             The stream request to check.
    \param  size            The pointer to receive the remaining amount of
                            bytes to transfer.

    \retval 0               On success.
    \retval -1              On failure.
*/
int syscall_gdrom_dma_check(gdc_cmd_hnd_t hnd, size_t *size);

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

    \param  hnd             The stream request to start transferring.
    \param  params          The pointer to a cd_transfer_params_t.

    \return                 0 on success, or non-zero on
                            failure.
*/
int syscall_gdrom_pio_transfer(gdc_cmd_hnd_t hnd, const cd_transfer_params_t *params);

/** \brief   Checks a GDROM PIO transfer.

    This function checks the progress of a PIO transfer for 
    \ref CMD_PIOREAD_STREAM_EX (see \ref dc/cdrom.h).

    \param  hnd             The stream request to check.
    \param  size            The pointer to receive the remaining amount of
                            bytes to transfer.

    \retval 0               On success.
    \retval -1              On failure.
*/
int syscall_gdrom_pio_check(gdc_cmd_hnd_t hnd, size_t *size);

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

