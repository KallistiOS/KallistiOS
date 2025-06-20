/* KallistiOS ##version##

   dc/cdrom.h
   Copyright (C) 2000-2001 Megan Potter
   Copyright (C) 2014 Donald Haase
   Copyright (C) 2023, 2024, 2025 Ruslan Rostovtsev
*/

#ifndef __DC_CDROM_H
#define __DC_CDROM_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <dc/syscalls.h>

/** \file    dc/cdrom.h
    \brief   CD access to the GD-ROM drive.
    \ingroup gdrom

    This file contains the interface to the Dreamcast's GD-ROM drive. It is
    simply called cdrom.h and cdrom.c because, by design, you cannot directly
    use this code to read the high-density area of GD-ROMs. This is the way it
    always has been, and always will be.

    The way things are set up, as long as you're using fs_iso9660 to access the
    CD, it will automatically detect and react to disc changes for you.

    This file only facilitates reading raw sectors and doing other fairly low-
    level things with CDs. If you're looking for higher-level stuff, like 
    normal file reading, consult with the stuff for the fs and for fs_iso9660.

    \author Megan Potter
    \author Ruslan Rostovtsev
    \see    kos/fs.h
    \see    dc/fs_iso9660.h
*/

/** \defgroup gdrom     GD-ROM 
    \brief              Driver for the Dreamcast's GD-ROM drive
    \ingroup            vfs
*/

/* These are defines provided for compatibility. These defines are now part of `cd_cmd_code_t` in dc/syscalls.h */
static const uint8_t  CMD_CHECK_LICENSE      __depr("Please use the new CD_ prefixed versions.") = CD_CMD_CHECK_LICENSE;
static const uint8_t  CMD_REQ_SPI_CMD        __depr("Please use the new CD_ prefixed versions.") = CD_CMD_REQ_SPI_CMD;
static const uint8_t  CMD_PIOREAD            __depr("Please use the new CD_ prefixed versions.") = CD_CMD_PIOREAD;
static const uint8_t  CMD_DMAREAD            __depr("Please use the new CD_ prefixed versions.") = CD_CMD_DMAREAD;
static const uint8_t  CMD_GETTOC             __depr("Please use the new CD_ prefixed versions.") = CD_CMD_GETTOC;
static const uint8_t  CMD_GETTOC2            __depr("Please use the new CD_ prefixed versions.") = CD_CMD_GETTOC2;
static const uint8_t  CMD_PLAY               __depr("Please use the new CD_ prefixed versions.") = CD_CMD_PLAY_TRACKS;
static const uint8_t  CMD_PLAY2              __depr("Please use the new CD_ prefixed versions.") = CD_CMD_PLAY_SECTORS;
static const uint8_t  CMD_PAUSE              __depr("Please use the new CD_ prefixed versions.") = CD_CMD_PAUSE;
static const uint8_t  CMD_RELEASE            __depr("Please use the new CD_ prefixed versions.") = CD_CMD_RELEASE;
static const uint8_t  CMD_INIT               __depr("Please use the new CD_ prefixed versions.") = CD_CMD_INIT;
static const uint8_t  CMD_DMA_ABORT          __depr("Please use the new CD_ prefixed versions.") = CD_CMD_DMA_ABORT;
static const uint8_t  CMD_OPEN_TRAY          __depr("Please use the new CD_ prefixed versions.") = CD_CMD_OPEN_TRAY;
static const uint8_t  CMD_SEEK               __depr("Please use the new CD_ prefixed versions.") = CD_CMD_SEEK;
static const uint8_t  CMD_DMAREAD_STREAM     __depr("Please use the new CD_ prefixed versions.") = CD_CMD_DMAREAD_STREAM;
static const uint8_t  CMD_NOP                __depr("Please use the new CD_ prefixed versions.") = CD_CMD_NOP;
static const uint8_t  CMD_REQ_MODE           __depr("Please use the new CD_ prefixed versions.") = CD_CMD_REQ_MODE;
static const uint8_t  CMD_SET_MODE           __depr("Please use the new CD_ prefixed versions.") = CD_CMD_SET_MODE;
static const uint8_t  CMD_SCAN_CD            __depr("Please use the new CD_ prefixed versions.") = CD_CMD_SCAN_CD;
static const uint8_t  CMD_STOP               __depr("Please use the new CD_ prefixed versions.") = CD_CMD_STOP;
static const uint8_t  CMD_GETSCD             __depr("Please use the new CD_ prefixed versions.") = CD_CMD_GETSCD;
static const uint8_t  CMD_GETSES             __depr("Please use the new CD_ prefixed versions.") = CD_CMD_GETSES;
static const uint8_t  CMD_REQ_STAT           __depr("Please use the new CD_ prefixed versions.") = CD_CMD_REQ_STAT;
static const uint8_t  CMD_PIOREAD_STREAM     __depr("Please use the new CD_ prefixed versions.") = CD_CMD_PIOREAD_STREAM;
static const uint8_t  CMD_DMAREAD_STREAM_EX  __depr("Please use the new CD_ prefixed versions.") = CD_CMD_DMAREAD_STREAM_EX;
static const uint8_t  CMD_PIOREAD_STREAM_EX  __depr("Please use the new CD_ prefixed versions.") = CD_CMD_PIOREAD_STREAM_EX;
static const uint8_t  CMD_GET_VERS           __depr("Please use the new CD_ prefixed versions.") = CD_CMD_GET_VERS;
static const uint8_t  CMD_MAX                __depr("Please use the new CD_ prefixed versions.") = CD_CMD_MAX;

/** \defgroup cd_cmd_response       Command Responses
    \brief                          Responses from GD-ROM syscalls
    \ingroup  gdrom

    These are the values that the various functions can return as error codes.
    @{
*/
#define ERR_OK          0   /**< \brief No error */
#define ERR_NO_DISC     1   /**< \brief No disc in drive */
#define ERR_DISC_CHG    2   /**< \brief Disc changed, but not reinitted yet */
#define ERR_SYS         3   /**< \brief System error */
#define ERR_ABORTED     4   /**< \brief Command aborted */
#define ERR_NO_ACTIVE   5   /**< \brief System inactive? */
#define ERR_TIMEOUT     6   /**< \brief Aborted due to timeout */
/** @} */

/** \defgroup cd_cmd_status         Command Status Responses
    \brief                          GD-ROM status responses
    \ingroup  gdrom

    These are the raw values the status syscall returns.
    @{
*/
#define FAILED      -1  /**< \brief Command failed */
#define NO_ACTIVE   0   /**< \brief System inactive? */
#define PROCESSING  1   /**< \brief Processing command */
#define COMPLETED   2   /**< \brief Command completed successfully */
#define STREAMING   3   /**< \brief Stream type command is in progress */
#define BUSY        4   /**< \brief GD syscalls is busy */
/** @} */

/** \defgroup cd_cmd_ata_status       ATA Statuses
    \brief                            ATA statuses for GD-ROM driver
    \ingroup  gdrom 

    @{
*/
#define ATA_STAT_INTERNAL   0x00
#define ATA_STAT_IRQ        0x01
#define ATA_STAT_DRQ_0      0x02
#define ATA_STAT_DRQ_1      0x03
#define ATA_STAT_BUSY       0x04
/** @} */

/** \defgroup cdda_read_modes       CDDA Read Modes
    \brief                          Read modes for CDDA
    \ingroup  gdrom

    Valid values to pass to the cdrom_cdda_play() function for the mode
    parameter.
    @{
*/
#define CDDA_TRACKS     1   /**< \brief Play by track number */
#define CDDA_SECTORS    2   /**< \brief Play by sector number */
/** @} */

/** \defgroup cd_read_sector_part    Read Sector Part
    \brief                           Whether to read data or whole sector
    \ingroup  gdrom

    Parts of the a CD-ROM sector to read. These are possible values for the
    third parameter word sent with the change data type syscall. 
    @{
*/
#define CDROM_READ_WHOLE_SECTOR 0x1000    /**< \brief Read the whole sector */
#define CDROM_READ_DATA_AREA    0x2000    /**< \brief Read the data area */
/** @} */

/** \defgroup cd_read_subcode_type    Read Subcode Type
    \brief                            Types of data to read from sector subcode
    \ingroup  gdrom

    Types of data available to read from the sector subcode. These are 
    possible values for the first parameter sent to the GETSCD syscall.
    @{
*/
#define CD_SUB_Q_ALL            0    /**< \brief Read all Subcode Data */
#define CD_SUB_Q_CHANNEL        1    /**< \brief Read Q Channel Subcode Data */
#define CD_SUB_MEDIA_CATALOG    2    /**< \brief Read the Media Catalog 
                                                 Subcode Data */
#define CD_SUB_TRACK_ISRC       3    /**< \brief Read the ISRC Subcode Data */
#define CD_SUB_RESERVED         4    /**< \brief Reserved */
/** @} */

/** \defgroup cd_subcode_audio    Subcode Audio Status
    \brief                        GETSCD syscall response codes
    \ingroup  gdrom

    Information about CDDA playback from GETSCD syscall.
    @{
*/
#define CD_SUB_AUDIO_STATUS_INVALID    0x00
#define CD_SUB_AUDIO_STATUS_PLAYING    0x11
#define CD_SUB_AUDIO_STATUS_PAUSED     0x12
#define CD_SUB_AUDIO_STATUS_ENDED      0x13
#define CD_SUB_AUDIO_STATUS_ERROR      0x14
#define CD_SUB_AUDIO_STATUS_NO_INFO    0x15
/** @} */

/* Compat. These got converted to a plain bool. */
static const bool  CDROM_READ_PIO   __depr("Please just use false to not use dma.") = false;
static const bool  CDROM_READ_DMA   __depr("Please just use true to use dma.") = true;

/* Compat. This can now be found in dc/syscalls.h */
#define CDROM_TOC __depr("Use the type cd_toc_t rather than CDROM_TOC.") cd_toc_t

/** \defgroup cd_toc_access         TOC Access Macros
    \brief                          Macros used to access the TOC
    \ingroup  gdrom

    @{
*/

/** \brief  Get the FAD address of a TOC entry.
    \param  n               The actual entry from the TOC to look at.
    \return                 The FAD of the entry.
*/
#define TOC_LBA(n) ((n) & 0x00ffffff)

/** \brief  Get the address of a TOC entry.
    \param  n               The entry from the TOC to look at.
    \return                 The entry's address.
*/
#define TOC_ADR(n) ( ((n) & 0x0f000000) >> 24 )

/** \brief  Get the control data of a TOC entry.
    \param  n               The entry from the TOC to look at.
    \return                 The entry's control value.
*/
#define TOC_CTRL(n) ( ((n) & 0xf0000000) >> 28 )

/** \brief  Get the track number of a TOC entry.
    \param  n               The entry from the TOC to look at.
    \return                 The entry's track.
*/
#define TOC_TRACK(n) ( ((n) & 0x00ff0000) >> 16 )
/** @} */

/** \brief  CD-ROM streams callback
*/
typedef void (*cdrom_stream_callback_t)(void *data);

/** \brief    Set the sector size for read sectors.
    \ingroup  gdrom

    This function sets the sector size that the cdrom_read_sectors() function
    will return. Be sure to set this to the correct value for the type of
    sectors you're trying to read. Common values are 2048 (for reading CD-ROM
    sectors) or 2352 (for reading raw sectors).

    \param  size            The size of the sector data.

    \return                 \ref cd_cmd_response
*/
int cdrom_set_sector_size(int size);

/** \brief    Execute a CD-ROM command.
    \ingroup  gdrom

    This function executes the specified command using the BIOS syscall for
    executing GD-ROM commands.

    \param  cmd             The command to execute.
    \param  param           Data to pass to the syscall.

    \return                 \ref cd_cmd_response
*/
int cdrom_exec_cmd(cd_cmd_code_t cmd, void *param);

/** \brief    Execute a CD-ROM command with timeout.
    \ingroup  gdrom

    This function executes the specified command using the BIOS syscall for
    executing GD-ROM commands with timeout.

    \param  cmd             The command to execute.
    \param  param           Data to pass to the syscall.
    \param  timeout         Timeout in milliseconds.

    \return                 \ref cd_cmd_response
*/
int cdrom_exec_cmd_timed(cd_cmd_code_t cmd, void *param, uint32_t timeout);

/** \brief    Abort a CD-ROM command with timeout.
    \ingroup  gdrom

    This function aborts current command using the BIOS syscall for
    aborting GD-ROM commands. They can also abort DMA transfers.

    \param  timeout         Timeout in milliseconds.
    \param  abort_dma       Whether to abort the DMA transfer.

    \return                 \ref cd_cmd_response
*/
int cdrom_abort_cmd(uint32_t timeout, bool abort_dma);

/** \brief    Get the status of the GD-ROM drive.
    \ingroup  gdrom

    \param  status          Space to return the drive's status.
    \param  disc_type       Space to return the type of disc in the drive.

    \return                 \ref cd_cmd_response
    \see    cd_status_values
    \see    cd_disc_types
*/
int cdrom_get_status(int *status, int *disc_type);

/** \brief    Change the datatype of disc.
    \ingroup  gdrom

    This function will take in all parameters to pass to the change_datatype 
    syscall. This allows these parameters to be modified without a reinit. 
    Each parameter allows -1 as a default, which is tied to the former static 
    values provided by cdrom_reinit and cdrom_set_sector_size.

    \param sector_part      How much of each sector to return.
    \param cdxa             What CDXA mode to read as (if applicable).
    \param sector_size      What sector size to read (eg. - 2048, 2532).

    \return                 \ref cd_cmd_response
    \see    cd_read_sector_part
*/
int cdrom_change_datatype(int sector_part, int cdxa, int sector_size);

/** \brief    Re-initialize the GD-ROM drive.
    \ingroup  gdrom

    This function is for reinitializing the GD-ROM drive after a disc change to
    its default settings. Calls cdrom_reinit(-1,-1,-1)

    \return                 \ref cd_cmd_response
    \see    cdrom_reinit_ex
*/
int cdrom_reinit(void);

/** \brief    Re-initialize the GD-ROM drive with custom parameters.
    \ingroup  gdrom

    At the end of each cdrom_reinit(), cdrom_change_datatype is called. 
    This passes in the requested values to that function after 
    reinitialization, as opposed to defaults.

    \param sector_part      How much of each sector to return.
    \param cdxa             What CDXA mode to read as (if applicable).
    \param sector_size      What sector size to read (eg. - 2048, 2532).

    \return                 \ref cd_cmd_response
    \see    cd_read_sector_part
    \see    cdrom_change_datatype
*/
int cdrom_reinit_ex(int sector_part, int cdxa, int sector_size);

/** \brief    Read the table of contents from the disc.
    \ingroup  gdrom

    This function reads the TOC from the specified area of the disc.
    On regular CD-ROMs, there are only low density area.

    \param  toc_buffer      Space to store the returned TOC in.
    \param  high_density    Whether to read from the high density area.
    \return                 \ref cd_cmd_response
*/
int cdrom_read_toc(cd_toc_t *toc_buffer, bool high_density);

/** \brief    Read one or more sector from a CD-ROM.
    \ingroup  gdrom

    This function reads the specified number of sectors from the disc, starting
    where requested. This will respect the size of the sectors set with
    cdrom_change_datatype(). The buffer must have enough space to store the
    specified number of sectors and size must be a multiple of 32 for DMA.

    \param  buffer          Space to store the read sectors.
    \param  sector          The sector to start reading from.
    \param  cnt             The number of sectors to read.
    \param  dma             True for read using dma, false for pio.
    \return                 \ref cd_cmd_response

    \note                   If the buffer address points to the P2 memory area,
                            the caller function will be responsible for ensuring
                            memory coherency.

*/
int cdrom_read_sectors_ex(void *buffer, uint32_t sector, size_t cnt, bool dma);

/** \brief    Read one or more sector from a CD-ROM in PIO mode.
    \ingroup  gdrom

    Default version of cdrom_read_sectors_ex, which forces PIO mode.

    \param  buffer          Space to store the read sectors.
    \param  sector          The sector to start reading from.
    \param  cnt             The number of sectors to read.
    \return                 \ref cd_cmd_response
    \see    cdrom_read_sectors_ex
*/
int cdrom_read_sectors(void *buffer, uint32_t sector, size_t cnt);

/** \brief    Start streaming from a CD-ROM.
    \ingroup  gdrom

    This function pre-reads the specified number of sectors from the disc.

    \param  sector          The sector to start reading from.
    \param  cnt             The number of sectors to read, 0x1ff means until end of disc.
    \param  dma             True for read using dma, false for pio.
    \return                 \ref cd_cmd_response
    \see    cdrom_transfer_request
*/
int cdrom_stream_start(int sector, int cnt, bool dma);

/** \brief    Stop streaming from a CD-ROM.
    \ingroup  gdrom

    This function finishing stream commands.

    \param  abort_dma       Abort current G1 DMA transfer.

    \return                 \ref cd_cmd_response
    \see    cdrom_transfer_request
*/
int cdrom_stream_stop(bool abort_dma);

/** \brief    Request stream transfer.
    \ingroup  gdrom

    This function request data from stream.

    \param  buffer          Space to store the read sectors (DMA aligned to 32, PIO to 2).
    \param  size            The size in bytes to read (DMA min 32, PIO min 2).
    \param  block           True to block until DMA transfer completes.
    \return                 \ref cd_cmd_response
    \see    cdrom_stream_start
*/
int cdrom_stream_request(void *buffer, size_t size, bool block);

/** \brief    Check requested stream transfer.
    \ingroup  gdrom

    This function check requested stream transfer.

    \param  size            The transfered (if in progress) or remain size in bytes.
    \return                 1 - is in progress, 0 - done
    \see    cdrom_transfer_request
*/
int cdrom_stream_progress(size_t *size);

/** \brief    Setting up a callback for transfers.
    \ingroup  gdrom

    This callback is called for every transfer request that is completed.

    \param  callback        Callback function.
    \param  param           Callback function param.
    \see    cdrom_transfer_request
*/
void cdrom_stream_set_callback(cdrom_stream_callback_t callback, void *param);

/** \brief    Read subcode data from the most recently read sectors.
    \ingroup  gdrom

    After reading sectors, this can pull subcode data regarding the sectors 
    read. If reading all subcode data with CD_SUB_CURRENT_POSITION, this needs 
    to be performed one sector at a time.

    \param  buffer          Space to store the read subcode data.
    \param  buflen          Amount of data to be read.
    \param  which           Which subcode type do you wish to get.

    \return                 \ref cd_cmd_response
    \see    cd_read_subcode_type
*/
int cdrom_get_subcode(void *buffer, int buflen, int which);

/** \brief    Locate the sector of the data track.
    \ingroup  gdrom

    This function will search the toc for the last entry that has a CTRL value
    of 4, and return its FAD address.

    \param  toc             The TOC to search through.
    \return                 The FAD of the track, or 0 if none is found.
*/
uint32 cdrom_locate_data_track(cd_toc_t *toc);

/** \brief    Play CDDA audio tracks or sectors.
    \ingroup  gdrom

    This function starts playback of CDDA audio.

    \param  start           The track or sector to start playback from.
    \param  end             The track or sector to end playback at.
    \param  loops           The number of times to repeat (max of 15).
    \param  mode            The mode to play (see \ref cdda_read_modes).
    \return                 \ref cd_cmd_response
*/
int cdrom_cdda_play(uint32_t start, uint32_t end, uint32_t loops, int mode);

/** \brief    Pause CDDA audio playback.
    \ingroup  gdrom

    \return                 \ref cd_cmd_response
*/
int cdrom_cdda_pause(void);

/** \brief    Resume CDDA audio playback after a pause.
    \ingroup  gdrom

    \return                 \ref cd_cmd_response
*/
int cdrom_cdda_resume(void);

/** \brief    Spin down the CD.
    \ingroup  gdrom

    This stops the disc in the drive from spinning until it is accessed again.

    \return                 \ref cd_cmd_response
*/
int cdrom_spin_down(void);

/** \brief    Initialize the GD-ROM for reading CDs.
    \ingroup  gdrom

    This initializes the CD-ROM reading system, reactivating the drive and
    handling initial setup of the disc.
*/
void cdrom_init(void);

/** \brief    Shutdown the CD reading system.
    \ingroup  gdrom
 */
void cdrom_shutdown(void);

__END_DECLS

#endif  /* __DC_CDROM_H */
