/* KallistiOS ##version##

   dc/cdrom.h
   Copyright (C) 2000-2001 Megan Potter
   Copyright (C) 2014, 2024 Donald Haase
   Copyright (C) 2023 Ruslan Rostovtsev
*/

#ifndef __DC_CDROM_H
#define __DC_CDROM_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
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
    \author Donald Haase
    \author Ruslan Rostovtsev
    \see    kos/fs.h
    \see    dc/fs_iso9660.h
*/

/** \defgroup gdrom     GD-ROM
    \brief              Driver for the Dreamcast's GD-ROM drive
    \ingroup            vfs
*/

/** \brief    Max times to retry a command */
#define CD_CMD_RETRY_MAX 10

/** \brief    Read modes for CDDA
    \ingroup  gdrom

    Valid values to pass to the cdrom_cdda_play() function for the mode
    parameter. These set whether to use
*/
typedef enum cd_cdda_mode {
    CDDA_TRACKS,   /**< \brief Play by track number (CMD_PLAY) */
    CDDA_SECTORS   /**< \brief Play by sector number (CMD_PLAY2) */
} cd_cdda_mode_t;

/** \brief      Mode to use when reading
    \ingroup    gdrom

    How to read the sectors of a CD, via PIO or DMA. 4th parameter of
    cdrom_read_sectors_ex.
*/
typedef enum cd_read_mode {
    CDROM_READ_PIO,    /**< \brief Read sector(s) in PIO mode */
    CDROM_READ_DMA     /**< \brief Read sector(s) in DMA mode */
} cd_read_mode_t;

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

    \return                 \ref cd_cmd_ret_t
*/
cd_cmd_ret_t cdrom_set_sector_size(size_t size);

/** \brief    Execute a CD-ROM command.
    \ingroup  gdrom

    This function executes the specified command using the BIOS syscall for
    executing GD-ROM commands.

    \param  cmd             The command number to execute.
    \param  param           Data to pass to the syscall.

    \return                 \ref cd_cmd_ret_t
*/
cd_cmd_ret_t cdrom_exec_cmd(cd_cmd_code_t cmd, void *param);

/** \brief    Execute a CD-ROM command with timeout.
    \ingroup  gdrom

    This function executes the specified command using the BIOS syscall for
    executing GD-ROM commands with timeout.

    \param  cmd             The command number to execute.
    \param  param           Data to pass to the syscall.
    \param  timeout         Timeout in milliseconds.

    \return                 \ref cd_cmd_ret_t
*/
cd_cmd_ret_t
cdrom_exec_cmd_timed(cd_cmd_code_t cmd, void *param, uint32_t timeout);

/** \brief    Abort a CD-ROM command.
    \ingroup  gdrom

    This function aborts the specified command using the BIOS syscall for
    aborting GD-ROM commands.

    \param  timeout         Timeout in milliseconds.
    \param  hnd             Handle of command to abort
    \return                 \ref cd_cmd_ret_t
*/
cd_cmd_ret_t cdrom_abort_cmd(uint32_t timeout, gdc_cmd_hnd_t hnd);

/** \brief    Get the status of the GD-ROM drive.
    \ingroup  gdrom
    
    This is a wrapper around syscall_gdrom_check_drive.
    
    TODO: this should be updated to just return what is taken without
    setting -1, with the return being used to determine if the params
    are valid or not.

    \param  status          Space to return the drive's status.
    \param  disc_type       Space to return the type of disc in the drive.

    \return                 \ref syscall_gdrom_check_drive
    \see    cd_status_values
    \see    cd_disc_types_t
*/
int cdrom_get_status(int *status, int *disc_type);

/** \brief    Change the datatype of disc.
    \ingroup  gdrom

    This function will take in all parameters to pass to the change_datatype 
    syscall. This allows these parameters to be modified without a reinit. 
    Each parameter allows -1 as a default, which is tied to the former static 
    values provided by cdrom_reinit and cdrom_set_sector_size.
    \param sector_part      How much of each sector to return.
    \param track_type       What track type to read as (if applicable).
    \param sector_size      What sector size to read (eg. - 2048, 2532).

    \return                 \ref syscall_gdrom_sector_mode
    \see    cd_read_sector_part
*/
int cdrom_change_datatype(cd_read_sec_part_t sector_part,
                      cd_track_type_t track_type, int sector_size);

/** \brief    Re-initialize the GD-ROM drive.
    \ingroup  gdrom

    This function is for reinitializing the GD-ROM drive after a disc change to
    its default settings. Calls cdrom_reinit(-1,-1,-1)

    \return                 \ref cd_cmd_ret_t
    \see    cdrom_reinit_ex
*/
cd_cmd_ret_t cdrom_reinit(void);

/** \brief    Re-initialize the GD-ROM drive with custom parameters.
    \ingroup  gdrom

    At the end of each cdrom_reinit(), cdrom_change_datatype is called.
    This passes in the requested values to that function after
    reinitialization, as opposed to defaults.

    \param sector_part      How much of each sector to return.
    \param track_type       What track type to read as (if applicable).
    \param sector_size      What sector size to read (eg. - 2048, 2532).

    \return                 \ref cd_cmd_ret_t
    \see    cd_read_sector_part
    \see    cdrom_change_datatype
*/
cd_cmd_ret_t
cdrom_reinit_ex(cd_read_sec_part_t sector_part,
                cd_track_type_t, int sector_size);

/** \brief    Read the table of contents from the disc.
    \ingroup  gdrom

    This function reads the TOC from the specified area of the disc.

    \param  toc_buffer      Space to store the returned TOC in.
    \param  area            The area of the disc to read (low = 0, high = 1).
    \return                 \ref cd_cmd_ret_t
*/
cd_cmd_ret_t cdrom_read_toc(cd_toc_t *toc_buffer, cd_area_t area);

/** \brief    Read one or more sector from a CD-ROM.
    \ingroup  gdrom

    This function reads the specified number of sectors from the disc, starting
    where requested. This will respect the size of the sectors set with
    cdrom_change_datatype(). The buffer must have enough space to store the
    specified number of sectors and size must be a multiple of 32 for DMA.

    \param  buffer          Space to store the read sectors.
    \param  sector          The sector to start reading from.
    \param  cnt             The number of sectors to read.
    \param  mode            \ref cd_read_mode_t
    \return                 \ref cd_cmd_ret_t
    \see    cd_read_sector_mode
*/
cd_cmd_ret_t
cdrom_read_sectors_ex(void *buffer, uint32_t sector, size_t cnt,
                      cd_read_mode_t mode);

/** \brief    Read one or more sector from a CD-ROM in PIO mode.
    \ingroup  gdrom

    Default version of cdrom_read_sectors_ex, which forces PIO mode.

    \param  buffer          Space to store the read sectors.
    \param  sector          The sector to start reading from.
    \param  cnt             The number of sectors to read.
    \return                 \ref cd_cmd_ret_t
    \see    cdrom_read_sectors_ex
*/
cd_cmd_ret_t
cdrom_read_sectors(void *buffer, uint32_t sector, size_t cnt);

/** \brief    Read subcode data from the most recently read sectors.
    \ingroup  gdrom

    After reading sectors, this can pull subcode data regarding the sectors
    read. If reading all subcode data with CD_SUB_CURRENT_POSITION, this needs
    to be performed one sector at a time.

    \param  buffer          Space to store the read subcode data.
    \param  buflen          Amount of data to be read.
    \param  which           Which subcode type do you wish to get.

    \return                 \ref cd_cmd_ret_t
    \see    cd_read_subcode_type
*/
cd_cmd_ret_t
cdrom_get_subcode(void *buffer, size_t buflen, cd_sub_type_t which);

/** \brief    Locate the sector of the data track.
    \ingroup  gdrom

    This function will search the toc for the last entry that has a CTRL value
    of 4, and return its FAD address.

    \param  toc             The TOC to search through.
    \return                 The FAD of the track, or 0 if none is found.
*/
uint32_t cdrom_locate_data_track(cd_toc_t *toc);

/** \brief    Play CDDA audio tracks or sectors.
    \ingroup  gdrom

    This function starts playback of CDDA audio.

    \param  start           The track or sector to start playback from.
    \param  end             The track or sector to end playback at.
    \param  loops           The number of times to repeat (max of 15).
    \param  mode            The mode to play (see \ref cdda_read_modes).
    \return                 \ref cd_cmd_ret_t
*/
cd_cmd_ret_t
cdrom_cdda_play(uint32_t start, uint32_t end, uint32_t loops, cd_cdda_mode_t mode);

/** \brief    Pause CDDA audio playback.
    \ingroup  gdrom

    \return                 \ref cd_cmd_ret_t
*/
cd_cmd_ret_t cdrom_cdda_pause(void);

/** \brief    Resume CDDA audio playback after a pause.
    \ingroup  gdrom

    \return                 \ref cd_cmd_ret_t
*/
cd_cmd_ret_t cdrom_cdda_resume(void);

/** \brief    Spin down the CD.
    \ingroup  gdrom

    This stops the disc in the drive from spinning until it is accessed again.

    \return                 \ref cd_cmd_ret_t
*/
cd_cmd_ret_t cdrom_spin_down(void);

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
