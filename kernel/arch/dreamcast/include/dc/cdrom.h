/* KallistiOS ##version##

   dc/cdrom.h
   Copyright (C) 2000-2001 Megan Potter
   Copyright (C) 2014 Donald Haase
   Copyright (C) 2023, 2024, 2025 Ruslan Rostovtsev
*/

#ifndef __DC_CDROM_H
#define __DC_CDROM_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <stdint.h>

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

/** \defgroup cd_cmd_codes          Syscall Command Codes
    \brief                          Command codes for GD-ROM syscalsl
    \ingroup  gdrom

    These are the syscall command codes used to actually do stuff with the
    GD-ROM drive. These were originally provided by maiwe.

    @{
*/
#define CMD_CHECK_LICENSE       2  /**< \brief Check license */
#define CMD_REQ_SPI_CMD         4  /**< \brief Request to Sega Packet Interface */
#define CMD_PIOREAD            16  /**< \brief Read via PIO */
#define CMD_DMAREAD            17  /**< \brief Read via DMA */
#define CMD_GETTOC             18  /**< \brief Read TOC */
#define CMD_GETTOC2            19  /**< \brief Read TOC */
#define CMD_PLAY               20  /**< \brief Play track */
#define CMD_PLAY2              21  /**< \brief Play sectors */
#define CMD_PAUSE              22  /**< \brief Pause playback */
#define CMD_RELEASE            23  /**< \brief Resume from pause */
#define CMD_INIT               24  /**< \brief Initialize the drive */
#define CMD_DMA_ABORT          25  /**< \brief Abort DMA transfer */
#define CMD_OPEN_TRAY          26  /**< \brief Open CD tray (on DevBox?) */
#define CMD_SEEK               27  /**< \brief Seek to a new position */
#define CMD_DMAREAD_STREAM     28  /**< \brief Stream DMA until end/abort */
#define CMD_NOP                29  /**< \brief No operation */
#define CMD_REQ_MODE           30  /**< \brief Request mode */
#define CMD_SET_MODE           31  /**< \brief Setup mode */
#define CMD_SCAN_CD            32  /**< \brief Scan CD */
#define CMD_STOP               33  /**< \brief Stop the disc from spinning */
#define CMD_GETSCD             34  /**< \brief Get subcode data */
#define CMD_GETSES             35  /**< \brief Get session */
#define CMD_REQ_STAT           36  /**< \brief Request stat */
#define CMD_PIOREAD_STREAM     37  /**< \brief Stream PIO until end/abort */
#define CMD_DMAREAD_STREAM_EX  38  /**< \brief Stream DMA transfer */
#define CMD_PIOREAD_STREAM_EX  39  /**< \brief Stream PIO transfer */
#define CMD_GET_VERS           40  /**< \brief Get syscall driver version */
#define CMD_MAX                47  /**< \brief Max of GD syscall commands */
/** @} */

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

/** \defgroup cd_read_sector_mode    Read Sector Mode
    \brief                           Mode to use when reading sectors
    \ingroup  gdrom

    How to read the sectors of a CD, via PIO or DMA. 4th parameter of 
    cdrom_read_sectors_ex.
    @{
*/
#define CDROM_READ_PIO 0    /**< \brief Read sector(s) in PIO mode */
#define CDROM_READ_DMA 1    /**< \brief Read sector(s) in DMA mode */
/** @} */

/** \defgroup cd_status_values      Status Values
    \brief                          Status values for GD-ROM drive
    \ingroup  gdrom

    These are the values that can be returned as the status parameter from the
    cdrom_get_status() function.
    @{
*/
#define CD_STATUS_READ_FAIL -1  /**< \brief Can't read status */
#define CD_STATUS_BUSY      0   /**< \brief Drive is busy */
#define CD_STATUS_PAUSED    1   /**< \brief Disc is paused */
#define CD_STATUS_STANDBY   2   /**< \brief Drive is in standby */
#define CD_STATUS_PLAYING   3   /**< \brief Drive is currently playing */
#define CD_STATUS_SEEKING   4   /**< \brief Drive is currently seeking */
#define CD_STATUS_SCANNING  5   /**< \brief Drive is scanning */
#define CD_STATUS_OPEN      6   /**< \brief Disc tray is open */
#define CD_STATUS_NO_DISC   7   /**< \brief No disc inserted */
#define CD_STATUS_RETRY     8   /**< \brief Retry is needed */
#define CD_STATUS_ERROR     9   /**< \brief System error */
#define CD_STATUS_FATAL     12  /**< \brief Need reset syscalls */
/** @} */

/** \defgroup cd_disc_types         Drive Disc Types
    \brief                          Disc types within GD-ROM drive
    \ingroup  gdrom

    These are the values that can be returned as the disc_type parameter from
    the cdrom_get_status() function.
    @{
*/
#define CD_CDDA     0x00    /**< \brief Audio CD (Red book) or no disc */
#define CD_CDROM    0x10    /**< \brief CD-ROM or CD-R (Yellow book) */
#define CD_CDROM_XA 0x20    /**< \brief CD-ROM XA (Yellow book extension) */
#define CD_CDI      0x30    /**< \brief CD-i (Green book) */
#define CD_GDROM    0x80    /**< \brief GD-ROM */
#define CD_FAIL     0xf0    /**< \brief Need reset syscalls */
/** @} */

/** \brief  TOC structure returned by the BIOS.
    \ingroup gdrom

    This is the structure that the CMD_GETTOC2 syscall command will return for
    the TOC. Note the data is in FAD, not LBA/LSN.

    \headerfile dc/cdrom.h
*/
typedef struct {
    uint32  entry[99];          /**< \brief TOC space for 99 tracks */
    uint32  first;              /**< \brief Point A0 information (1st track) */
    uint32  last;               /**< \brief Point A1 information (last track) */
    uint32  leadout_sector;     /**< \brief Point A2 information (leadout) */
} CDROM_TOC;

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

    \param  cmd             The command number to execute.
    \param  param           Data to pass to the syscall.

    \return                 \ref cd_cmd_response
*/
int cdrom_exec_cmd(int cmd, void *param);

/** \brief    Execute a CD-ROM command with timeout.
    \ingroup  gdrom

    This function executes the specified command using the BIOS syscall for
    executing GD-ROM commands with timeout.

    \param  cmd             The command number to execute.
    \param  param           Data to pass to the syscall.
    \param  timeout         Timeout in milliseconds.

    \return                 \ref cd_cmd_response
*/
int cdrom_exec_cmd_timed(int cmd, void *param, uint32_t timeout);

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

    \note                   This function is formally deprecated. It should not
                            be used in any future code, and may be removed in
                            the future. You should instead use
                            cdrom_change_datatype.
*/
int cdrom_change_dataype(int sector_part, int cdxa, int sector_size)
                        __depr("Use cdrom_change_datatype instead.");

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

    This function reads the TOC from the specified session of the disc.

    \param  toc_buffer      Space to store the returned TOC in.
    \param  session         The session of the disc to read.
    \return                 \ref cd_cmd_response
*/
int cdrom_read_toc(CDROM_TOC *toc_buffer, int session);

/** \brief    Read one or more sector from a CD-ROM.
    \ingroup  gdrom

    This function reads the specified number of sectors from the disc, starting
    where requested. This will respect the size of the sectors set with
    cdrom_change_datatype(). The buffer must have enough space to store the
    specified number of sectors and size must be a multiple of 32 for DMA.

    \param  buffer          Space to store the read sectors.
    \param  sector          The sector to start reading from.
    \param  cnt             The number of sectors to read.
    \param  mode            \ref cd_read_sector_mode
    \return                 \ref cd_cmd_response

    \note                   If the buffer address points to the P2 memory area,
                            the caller function will be responsible for ensuring
                            memory coherency.

    \see    cd_read_sector_mode
*/
int cdrom_read_sectors_ex(void *buffer, int sector, int cnt, int mode);

/** \brief    Read one or more sector from a CD-ROM in PIO mode.
    \ingroup  gdrom

    Default version of cdrom_read_sectors_ex, which forces PIO mode.

    \param  buffer          Space to store the read sectors.
    \param  sector          The sector to start reading from.
    \param  cnt             The number of sectors to read.
    \return                 \ref cd_cmd_response
    \see    cdrom_read_sectors_ex
*/
int cdrom_read_sectors(void *buffer, int sector, int cnt);

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
uint32 cdrom_locate_data_track(CDROM_TOC *toc);

/** \brief    Play CDDA audio tracks or sectors.
    \ingroup  gdrom

    This function starts playback of CDDA audio.

    \param  start           The track or sector to start playback from.
    \param  end             The track or sector to end playback at.
    \param  loops           The number of times to repeat (max of 15).
    \param  mode            The mode to play (see \ref cdda_read_modes).
    \return                 \ref cd_cmd_response
*/
int cdrom_cdda_play(uint32 start, uint32 end, uint32 loops, int mode);

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
