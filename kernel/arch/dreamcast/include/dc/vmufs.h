/* KallistiOS ##version##

   dc/vmufs.h
   Copyright (C) 2003 Megan Potter

*/

/** \file    dc/vmufs.h
    \brief   Low-level VMU filesystem driver.
    \ingroup vfs_vmu

    The VMU filesystem driver mounts itself on /vmu of the VFS. Each memory card
    has its own subdirectory off of that directory (i.e, /vmu/a1 for slot 1 of
    the first controller). VMUs themselves have no subdirectories, so the driver
    itself is fairly simple.

    Files on a VMU must be multiples of 512 bytes in size, and should have a
    header attached so that they show up in the BIOS menu.

    \author Megan Potter
    \see    dc/vmu_pkg.h
    \see    dc/fs_vmu.h
*/

#ifndef __DC_VMUFS_H
#define __DC_VMUFS_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <dc/maple.h>

/** \addtogroup vfs_vmu
    @{
*/

/** \brief  BCD timestamp, used several places in the vmufs.
    \headerfile dc/vmufs.h
*/
typedef struct {
    uint8   cent;   /**< \brief Century */
    uint8   year;   /**< \brief Year, within century */
    uint8   month;  /**< \brief Month of the year */
    uint8   day;    /**< \brief Day of the month */
    uint8   hour;   /**< \brief Hour of the day */
    uint8   min;    /**< \brief Minutes */
    uint8   sec;    /**< \brief Seconds */
    uint8   dow;    /**< \brief Day of week (0 = monday, etc) */
} vmu_timestamp_t;

/** \brief  VMU FS Root block layout.
    \headerfile dc/vmufs.h
*/
typedef struct {
    uint8           magic[16];      /**< \brief All should contain 0x55 */
    uint8           use_custom;     /**< \brief 0 = standard, 1 = custom */
    uint8           custom_color[4];/**< \brief blue, green, red, alpha */
    uint8           pad1[27];       /**< \brief All zeros */
    vmu_timestamp_t timestamp;      /**< \brief BCD timestamp */
    uint8           pad2[8];        /**< \brief All zeros */
    uint8           unk1[6];        /**< \brief ??? */
    uint16          fat_loc;        /**< \brief FAT location */
    uint16          fat_size;       /**< \brief FAT size in blocks */
    uint16          dir_loc;        /**< \brief Directory location */
    uint16          dir_size;       /**< \brief Directory size in blocks */
    uint16          icon_shape;     /**< \brief Icon shape for this VMS */
    uint16          blk_cnt;        /**< \brief Number of user blocks */
    uint8           unk2[430];      /**< \brief ??? */
} vmu_root_t;

/** \brief  VMU FS Directory entries, 32 bytes each.
    \headerfile dc/vmufs.h
*/
typedef struct {
    uint8           filetype;       /**< \brief 0x00 = no file; 0x33 = data; 0xcc = a game */
    uint8           copyprotect;    /**< \brief 0x00 = copyable; 0xff = copy protected */
    uint16          firstblk;       /**< \brief Location of the first block in the file */
    char            filename[12];   /**< \brief Note: there is no null terminator */
    vmu_timestamp_t timestamp;      /**< \brief File time */
    uint16          filesize;       /**< \brief Size of the file in blocks */
    uint16          hdroff;         /**< \brief Offset of header, in blocks from start of file */
    uint8           dirty;          /**< \brief See header notes */
    uint8           pad1[3];        /**< \brief All zeros */
} vmu_dir_t;

/* Notes about the "dirty" field on vmu_dir_t :)

   This byte should always be zero when written out to the VMU. What this
   lets us do, though, is conserve on flash writes. If you only want to
   modify one single file (which is the standard case) then re-writing all
   of the dir blocks is a big waste. Instead, you should set the dirty flag
   on the in-mem copy of the directory, and writing it back out will only
   flush the containing block back to the VMU, setting it back to zero
   in the process. Loaded blocks should always have zero here (though we
   enforce that in the code to make sure) so it will be non-dirty by
   default.
 */


/* ****************** Low level functions ******************** */

/** \brief  Fill in the date on a vmu_dir_t for writing.

    \param  d               The directory to fill in the date on.
*/
void vmufs_dir_fill_time(vmu_dir_t *d);

/** \brief  Reads a selected VMU's root block.

    This function assumes the mutex is held.

    \param  dev             The VMU to read from.
    \param  root_buf        A buffer to hold the root block. You must allocate
                            this yourself before calling.
    \retval -1              On failure.
    \retval 0               On success.
*/
int vmufs_root_read(maple_device_t * dev, vmu_root_t * root_buf);

/** \brief  Writes a selected VMU's root block.

    This function assumes the mutex is held.

    \param  dev             The VMU to write to.
    \param  root_buf        The root block to write.
    \retval -1              On failure.
    \retval 0               On success.
*/
int vmufs_root_write(maple_device_t * dev, vmu_root_t * root_buf);

/** \brief  Given a VMU's root block, return the amount of space in bytes
            required to hold its directory.

    \param  root_buf        The root block to check.
    \return                 The amount of space, in bytes, needed.
*/
int vmufs_dir_blocks(vmu_root_t * root_buf);

/** \brief  Given a VMU's root block, return the amount of space in bytes
            required to hold its FAT.

    \param  root_buf        The root block to check.
    \return                 The amount of space, in bytes, needed.
*/
int vmufs_fat_blocks(vmu_root_t * root_buf);

/** \brief  Given a selected VMU's root block, read its directory.

    This function reads the directory of a given VMU root block. It assumes the
    mutex is held. There must be at least the number of bytes returned by
    vmufs_dir_blocks() available in the buffer for this to succeed.

    \param  dev             The VMU to read.
    \param  root_buf        The VMU's root block.
    \param  dir_buf         The buffer to hold the directory. You must have
                            allocated this yourself.
    \return                 0 on success, <0 on failure.
*/
int vmufs_dir_read(maple_device_t * dev, vmu_root_t * root_buf,
                   vmu_dir_t * dir_buf);

/** \brief  Given a selected VMU's root block and dir blocks, write the dirty
            dir blocks back to the VMU. Assumes the mutex is held.

    \param  dev             The VMU to write to.
    \param  root            The VMU's root block.
    \param  dir_buf         The VMU's directory structure.
    \return                 0 on success, <0 on failure.
*/
int vmufs_dir_write(maple_device_t * dev, vmu_root_t * root,
                    vmu_dir_t * dir_buf);

/** \brief Given a selected VMU's root block, read its FAT.

    This function reads the FAT of a VMU, given its root block. It assumes the
    mutex is held. There must be at least the number of bytes returned by
    vmufs_fat_blocks() available in the buffer for this to succeed.

    \param  dev             The VMU to read from.
    \param  root            The VMU's root block.
    \param  fat_buf         The buffer to store the FAT into. You must
                            pre-allocate this.
    \return                 0 on success, <0 on failure.
*/
int vmufs_fat_read(maple_device_t * dev, vmu_root_t * root, uint16 * fat_buf);

/** \brief  Given a selected VMU's root block and its FAT, write the FAT blocks
            back to the VMU.

    This function assumes the mutex is held.

    \param  dev             The VMU to write to.
    \param  root            The VMU's root block.
    \param  fat_buf         The buffer to write to the FAT.
    \return                 0 on success, <0 on failure.
*/
int vmufs_fat_write(maple_device_t * dev, vmu_root_t * root, uint16 * fat_buf);

/** \brief  Given a previously-read directory, locate a file by filename.

    \param  root            The VMU root block.
    \param  dir             The VMU directory.
    \param  fn              The file to find (only checked up to 12 chars).
    \return                 The index into the directory array on success, or
                            <0 on failure.
*/
int vmufs_dir_find(vmu_root_t * root, vmu_dir_t * dir, const char * fn);

/** \brief  Given a previously-read directory, add a new dirent to the dir.

    Another file with the same name should not exist (delete it first if it
    does). This function will not check for dupes!

    \param  root            The VMU root block.
    \param  dir             The VMU directory.
    \param  newdirent       The new entry to add.
    \return                 0 on success, or <0 on failure. */
int vmufs_dir_add(vmu_root_t * root, vmu_dir_t * dir, vmu_dir_t * newdirent);

/** \brief  Given a pointer to a directory struct and a previously loaded FAT,
            load the indicated file from the VMU.

    An appropriate amount of space must have been allocated previously in the
    buffer. Assumes the mutex is held.

    \param  dev             The VMU to read from.
    \param  fat             The FAT of the VMU.
    \param  dirent          The entry to read.
    \param  outbuf          A buffer to write the data into. You must allocate
                            this yourself with the appropriate amount of space.
    \return                 0 on success, <0 on failure.
*/
int vmufs_file_read(maple_device_t * dev, uint16 * fat, vmu_dir_t * dirent, void * outbuf);

/** \brief  Given a pointer to a mostly-filled directory struct and a previously
            loaded directory and FAT, write the indicated file to the VMU.

    The named file should not exist in the directory already. The directory and
    FAT will _not_ be sync'd back to the VMU, this must be done manually.
    Assumes the mutex is held.

    \param  dev             The VMU to write to.
    \param  root            The VMU root block.
    \param  fat             The FAT of the VMU.
    \param  dir             The directory of the VMU.
    \param  newdirent       The new entry to write.
    \param  filebuf         The new file data.
    \param  size            The size of the file in blocks (512-bytes each).
    \return                 0 on success, <0 on failure.
*/
int vmufs_file_write(maple_device_t * dev, vmu_root_t * root, uint16 * fat,
                     vmu_dir_t * dir, vmu_dir_t * newdirent, void * filebuf, int size);

/** \brief  Given a previously-read FAT and directory, delete the named file.

    No changes are made to the VMU itself, just the in-memory structs.

    \param  root            The VMU root block.
    \param  fat             The FAT to be modified.
    \param  dir             The directory to be modified.
    \param  fn              The file name to be deleted.
    \retval 0               On success.
    \retval -1              If fn is not found.
*/
int vmufs_file_delete(vmu_root_t * root, uint16 * fat, vmu_dir_t * dir, const char *fn);

/** \brief  Given a previously-read FAT, return the number of blocks available
            to write out new file data.

    \param  root            The VMU root block.
    \param  fat             The FAT to be examined.
    \return                 The number of blocks available.
*/
int vmufs_fat_free(vmu_root_t * root, uint16 * fat);

/** \brief  Given a previously-read directory, return the number of dirents
            available for new files.

    \param  root            The VMU root block.
    \param  dir             The directory in question.
    \return                 The number of entries available.
*/
int vmufs_dir_free(vmu_root_t * root, vmu_dir_t * dir);

/** \brief  Lock the vmufs mutex.

    This should be done before you attempt any low-level ops.

    \retval 0               On success (no error conditions defined).
*/
int vmufs_mutex_lock(void);

/** \brief  Unlock the vmufs mutex.

    This should be done once you're done with any low-level ops.

    \retval 0               On success (no error conditions defined).
*/
int vmufs_mutex_unlock(void);


/* ****************** Higher level functions ******************** */

/** \brief  Read the directory from a VMU.

    The output buffer will be allocated for you using malloc(), and the number
    of entries will be returned. On failure, outbuf will not contain a dangling
    buffer that needs to be freed (no further action required).

    \param  dev             The VMU to read from.
    \param  outbuf          A buffer that will be allocated where the directory
                            data will be placed.
    \param  outcnt          The number of entries in outbuf.
    \return                 0 on success, or <0 on failure. */
int vmufs_readdir(maple_device_t * dev, vmu_dir_t ** outbuf, int * outcnt);

/** \brief  Read a file from the VMU.

    The output buffer will be allocated for you using malloc(), and the size of
    the file will be returned.  On failure, outbuf will not contain a dangling
    buffer that needs to be freed (no further action required).

    \param  dev             The VMU to read from.
    \param  fn              The name of the file to read.
    \param  outbuf          A buffer that will be allocated where the file data
                            will be placed.
    \param  outsize         Storage for the size of the file, in bytes.
    \return                 0 on success, or <0 on failure.
*/
int vmufs_read(maple_device_t * dev, const char * fn, void ** outbuf, int * outsize);

/** \brief  Read a file from the VMU, using a pre-read dirent.

    This function is faster to use than vmufs_read() if you already have done
    the lookup, since it won't need to do that.

    \param  dev             The VMU to read from.
    \param  dirent          The entry to read.
    \param  outbuf          A buffer that will be allocated where the file data
                            will be placed.
    \param  outsize         Storage for the size of the file, in bytes.
    \return                 0 on success, <0 on failure.
*/
int vmufs_read_dirent(maple_device_t * dev, vmu_dir_t * dirent, void ** outbuf, int * outsize);

/* Flags for vmufs_write */
#define VMUFS_OVERWRITE 1   /**< \brief Overwrite existing files */
#define VMUFS_VMUGAME   2   /**< \brief This file is a VMU game */
#define VMUFS_NOCOPY    4   /**< \brief Set the no-copy flag */

/** \brief Write a file to the VMU.

    If the named file already exists, then the function checks 'flags'. If
    VMUFS_OVERWRITE is set, then the old file is deleted first before the new
    one is written (this all happens atomically). On partial failure, some data
    blocks may have been written, but in general the card should not be damaged.

    \param  dev             The VMU to write to.
    \param  fn              The filename to write.
    \param  inbuf           The data to write to the file.
    \param  insize          The size of the file in bytes.
    \param  flags           Flags for the write (i.e, VMUFS_OVERWRITE,
                            VMUFS_VMUGAME, VMUFS_NOCOPY).
    \return                 0 on success, or <0 for failure.
*/
int vmufs_write(maple_device_t * dev, const char * fn, void * inbuf, int insize, int flags);

/** \brief  Delete a file from the VMU.

    \retval 0               On success.
    \retval -1              If the file is not found.
    \retval -2              On other failure.
*/
int vmufs_delete(maple_device_t * dev, const char * fn);

/** \brief  Return the number of user blocks free for file writing.

    You should check this number before attempting to write.

    \return                 The number of blocks free for writing.
*/
int vmufs_free_blocks(maple_device_t * dev);


/** \brief  Initialize vmufs.

    Must be called before anything else is useful.

    \retval 0               On success (no error conditions defined).
*/
int vmufs_init(void);

/** \brief  Shutdown vmufs.

    Must be called after everything is finished.
*/
int vmufs_shutdown(void);

/** @} */

__END_DECLS

#endif  /* __DC_VMUFS_H */
