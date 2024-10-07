/* KallistiOS ##version##

   dc/vmufs.h
   Copyright (C) 2003 Megan Potter
   Copyright (C) 2023 Falco Girgis

*/

/** \file    dc/vmufs.h
    \brief   Low-level VMU filesystem driver.
<<<<<<< HEAD
    \ingroup vmufs
=======
    \ingroup vfs_vmu
>>>>>>> master

    The VMU filesystem driver mounts itself on /vmu of the VFS. Each memory card
    has its own subdirectory off of that directory (i.e, /vmu/a1 for slot 1 of
    the first controller). VMUs themselves have no subdirectories, so the driver
    itself is fairly simple.

    Files on a VMU must be multiples of 512 bytes (1 block) in size, and should
    have a header attached so that they show up in the BIOS menu.

    \sa    dc/vmu_pkg.h
    \sa    dc/fs_vmu.h
    \sa    dc/maple/vmu.h
    \sa    dc/vmu_fb.h

    \author Megan Potter
    \author Falco Girgis
*/

#ifndef __DC_VMUFS_H
#define __DC_VMUFS_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#include <dc/maple.h>

/** \defgroup vmufs VMU Filesystem
    \brief    Low-level VMU Filesystem Driver

    \sa vmu
*/

#define VMU_BLOCK_SIZE      512
#define VMU_FILENAME_SIZE   12

#define VMU_FILE_NONE       0x00
#define VMU_FILE_DATA       0x33
#define VMU_FILE_GAME       0xcc

#define VMU_FILE_COPYABLE   0x00
#define VMU_FILE_PROTECTED  0xff

#define VMU_ROOT_MAGIC      0x55
#define VMU_ROOT_MAGIC_SIZE 16

/* Flags for vmufs_write */
#define VMU_OVERWRITE     1   /**< \brief Overwrite existing files */
#define VMU_VMUGAME       2   /**< \brief This file is a VMU game */
#define VMU_NOCOPY        4   /**< \brief Set the no-copy flag */

#define VMU_FAT_UNALLOCATED   0xfffc
#define VMU_FAT_LAST_IN_FILE  0xfffa
#define VMU_FAT_DAMAGED       0xffff

/** \brief   VMU Block Number
    \ingroup vmufs

    Typedef representing a single VMU block.
*/
typedef uint16_t vmu_block_t;

/** \brief   VMU Storge Media Information
    \ingroup vmufs

    Structure used to represent the volume information held by a MEMCARD
    partition as returned by either the Maple "Get_Media_Info" command or
    as stored within the FAT root block.

    The information describes the size, location, and layout of every segment
    within the filesystem.

    \sa vmu_root

*/
typedef struct vmu_media_info {
    vmu_block_t     total_size;    /**< \brief Total partition size in blocks (default: 255) */
    uint16_t        partition;     /**< \brief Partition number (default: 0) */
    vmu_block_t     root_loc;      /**< \brief Location of root block (default: 255) */
    vmu_block_t     fat_loc;       /**< \brief FAT location (default: 254) */
    vmu_block_t     fat_size;      /**< \brief FAT size in blocks (default: 1) */
    vmu_block_t     dir_loc;       /**< \brief Directory location (default: 253) */
    vmu_block_t     dir_size;      /**< \brief Directory size in blocks (default: 13) */
    uint8_t         icon_shape;    /**< \brief Icon shape for this VMS (0-123)*/
    uint8_t         extra_flag;    /**< \brief ??? */
    vmu_block_t     hidden_loc;    /**< \brief Hidden region location (default: 200) */
    vmu_block_t     hidden_size;   /**< \brief Hidden region size in blocks (default: 41) */
    vmu_block_t     game_loc;      /**< \brief Game location (default: 0) */
    vmu_block_t     game_size;     /**< \brief Game size in blocks (default: 128?) */
} vmu_media_info_t;

STATIC_ASSERT(sizeof(vmu_media_info_t) == 24,
              "vmu_media_info structureis not 24 bytes!");

/** \brief   VMU Volume Label
    \ingroup vmufs

*/
typedef struct vmu_volume_label {
    uint8_t         use_custom_color; /**< \brief 0 = no, 1 = custom color */
    union {                           /**< \brief Custom color format union */
        uint8_t     custom_color[4];  /**< \brief Custom color (BGRA8888) */
        struct {                      /**< \brief Custom color channels */
            uint8_t blue;             /**< \brief Custom color blue channel */
            uint8_t green;            /**< \brief Custom color green channel */
            uint8_t red;              /**< \brief Custom color red channel */
            uint8_t alpha;            /**< \brief Custom color alpha channel */
        };
    };
    uint8_t         unused[27];       /**< \brief Extra storage (Default: all 0s) */
} vmu_volume_label_t;

STATIC_ASSERT(sizeof(vmu_volume_label_t) == 32,
              "vmu_volume_label structure is not 32 bytes!");

/** \brief   BCD timestamp, used several places in the vmufs.
    \ingroup vmufs
*/
typedef struct vmu_timestamp {
    uint8_t   cent;   /**< \brief Century (0-99) */
    uint8_t   year;   /**< \brief Year, within century (0-99) */
    uint8_t   month;  /**< \brief Month of the year (1-12) */
    uint8_t   day;    /**< \brief Day of the month (1-31) */
    uint8_t   hour;   /**< \brief Hour of the day (0-23) */
    uint8_t   min;    /**< \brief Minutes (0-59) */
    uint8_t   sec;    /**< \brief Seconds (0-59) */
    uint8_t   dow;    /**< \brief Day of week (0 = Mon, ..., 6 = Sun) */
} vmu_timestamp_t;

STATIC_ASSERT(sizeof(vmu_timestamp_t) == 32,
              "vmu_timestamp structure is not 8 bytes!");

/** \brief   VMU FS Root block layout.
    \ingroup vmufs
*/
typedef struct vmu_root {
    /** \brief All should contain \ref VMU_MAGIC. */
    uint8_t            magic[VMU_ROOT_MAGIC_SIZE];

    /** \brief Volume Label */
    vmu_volume_label_t volume_label;

    /** \brief BCD timestamp when formatted */
    vmu_timestamp_t    timestamp;

    /** \brief Reserved (Default: All 0s) */
    uint8_t            reserved1[8];

    /** \brief Partition information */
    vmu_media_info_t   media_info;

    /** \brief Reserved (Default: All 0s) */
    uint8_t            reserved2[8];

    /** \brief Unused remainder of root block */
    uint8_t            reserved3[0x1f0 - 0x60];
} __packed vmu_root_t;

STATIC_ASSERT(sizeof(vmu_root_t) == 32,
              "vmu_root structure is not 512 bytes!");

/** \brief   VMU FS Directory entries, 32 bytes each.
    \ingroup vmufs

    \note
    vmu_dir_t::dirty should always be zero when written out to the VMU. What this
    lets us do, though, is conserve on flash writes. If you only want to
    modify one single file (which is the standard case) then re-writing all
    of the dir blocks is a big waste. Instead, you should set the dirty flag
    on the in-mem copy of the directory, and writing it back out will only
    flush the containing block back to the VMU, setting it back to zero
    in the process. Loaded blocks should always have zero here (though we
    enforce that in the code to make sure) so it will be non-dirty by
    default.
*/
typedef struct vmu_dir {
    uint8_t         filetype;                    /**< \brief 0x00 = no file; 0x33 = data; 0xcc = a game */
    uint8_t         copyprotect;                 /**< \brief 0x00 = copyable; 0xff = copy protected */
    vmu_block_t     firstblk;                    /**< \brief Location of the first block in the file */
    char            filename[VMU_FILENAME_SIZE]; /**< \brief File name in SHIFT-JS (No NULL terminator!) */
    vmu_timestamp_t timestamp;                   /**< \brief File time */
    vmu_block_t     filesize;                    /**< \brief Size of the file in blocks */
    vmu_block_t     hdroff;                      /**< \brief Offset of header, in blocks from start of file */
    uint8_t         dirty;                       /**< \brief See header notes */
    uint8_t         pad1[3];                     /**< \brief All zeros */
} vmu_dir_t;

STATIC_ASSERT(sizeof(vmu_dir_t) == 32,
              "vmu_dir structure is not 32 bytes!");;

/* ****************** Low level functions ******************** */

time_t vmufs_timestamp_to_unix(const vmu_timestamp_t *timestamp);

int vmufs_timestamp_from_unix(vmu_timestamp_t *timestamp, time_t unix);

void vmu_media_info_user_region(const vmu_media_info_t *info, vmu_block_t *user_loc, vmu_block_t *user_size);

/** \brief  Fill in the date on a vmu_dir_t for writing.

    \param  d               The directory to fill in the date on.
    \retval                 The current date/time which was filled.
    \retval                 -1 upon error
*/
time_t vmufs_dir_fill_time(vmu_dir_t *d);

int vmufs_format(maple_device_t *dev);

int vmufs_unformat(maple_device_t *dev);

int vmufs_check_formatted();

int vmufs_extra_blocks();

int vmufs_damaged_blocks();

int vmufs_save();

int vmufs_load();

int vmufs_defrag();

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

/** \brief  Reads a selected VMU's root block.

    This function assumes the mutex is held.

    \param  dev             The VMU to read from.
    \param  root_buf        A buffer to hold the root block. You must allocate
                            this yourself before calling.
    \retval -1              On failure.
    \retval 0               On success.
*/
int vmufs_root_read(maple_device_t *dev, vmu_root_t *root_buf);

/** \brief  Writes a selected VMU's root block.

    This function assumes the mutex is held.

    \param  dev             The VMU to write to.
    \param  root_buf        The root block to write.
    \retval -1              On failure.
    \retval 0               On success.
*/
int vmufs_root_write(maple_device_t *dev, const vmu_root_t *root_buf);

/** \brief  Given a VMU's root block, return the amount of space in bytes
            required to hold its FAT.

    \param  root_buf        The root block to check.
    \return                 The amount of space, in bytes, needed.
*/
size_t vmufs_fat_bytes(const vmu_root_t *root_buf);

/** \brief Given a selected VMU's root block, read its FAT.

    This function reads the FAT of a VMU, given its root block. It assumes the
    mutex is held. There must be at least the number of bytes returned by
    vmufs_fat_bytes() available in the buffer for this to succeed.

    \param  dev             The VMU to read from.
    \param  root            The VMU's root block.
    \param  fat_buf         The buffer to store the FAT into. You must
                            pre-allocate this.
    \return                 0 on success, <0 on failure.
*/
int vmufs_fat_read(maple_device_t *dev, const vmu_root_t *root,
                   vmu_block_t *fat_buf);

/** \brief  Given a selected VMU's root block and its FAT, write the FAT blocks
            back to the VMU.

    This function assumes the mutex is held.

    \param  dev             The VMU to write to.
    \param  root            The VMU's root block.
    \param  fat_buf         The buffer to write to the FAT.
    \return                 0 on success, <0 on failure.
*/
int vmufs_fat_write(maple_device_t *dev, const vmu_root_t *root,
                    const vmu_block_t *fat_buf);

/** \brief  Given a previously-read FAT, return the number of blocks available
            to write out new file data.

    \param  root            The VMU root block.
    \param  fat             The FAT to be examined.
    \return                 The number of blocks available.
*/
size_t vmufs_fat_free(const vmu_root_t *root, const vmu_block_t *fat);

/** \brief  Given a VMU's root block, return the amount of space in bytes
            required to hold its directory.

    \param  root_buf        The root block to check.
    \return                 The amount of space, in bytes, needed.
*/
size_t vmufs_dir_bytes(const vmu_root_t *root_buf);

/** \brief  Given a selected VMU's root block, read its directory.

    This function reads the directory of a given VMU root block. It assumes the
    mutex is held. There must be at least the number of bytes returned by
    vmufs_dir_bytes() available in the buffer for this to succeed.

    \param  dev             The VMU to read.
    \param  root_buf        The VMU's root block.
    \param  dir_buf         The buffer to hold the directory. You must have
                            allocated this yourself.
    \return                 0 on success, <0 on failure.
*/
int vmufs_dir_read(maple_device_t *dev, const vmu_root_t *root_buf,
                   vmu_dir_t *dir_buf);

/** \brief  Given a selected VMU's root block and dir blocks, write the dirty
            dir blocks back to the VMU. Assumes the mutex is held.

    \param  dev             The VMU to write to.
    \param  root            The VMU's root block.
    \param  dir_buf         The VMU's directory structure.
    \return                 0 on success, <0 on failure.
*/
int vmufs_dir_write(maple_device_t *dev, const vmu_root_t *root,
                    const vmu_dir_t *dir_buf);

/** \brief  Given a previously-read directory, locate a file by filename.

    \param  root            The VMU root block.
    \param  dir             The VMU directory.
    \param  fn              The file to find (only checked up to 12 chars).
    \return                 The index into the directory array on success, or
                            <0 on failure.
*/
ssize_t vmufs_dir_find(const vmu_root_t *root, const vmu_dir_t *dir,
                       const char *fn);

/** \brief  Given a previously-read directory, locate the GAME file.

    \param  root            The VMU root block.
    \param  dir             The VMU directory.
    \return                 The index into the directory array on success, or
                            <0 on failure.
*/
ssize_t vmufs_dir_game(const vmu_root_t *root, const vmu_dir_t *dir);

/** \brief  Given a previously-read directory, locate the ICONDATA_VMS file.

    \param  root            The VMU root block.
    \param  dir             The VMU directory.
    \return                 The index into the directory array on success, or
                            <0 on failure.
*/
ssize_t vmufs_dir_icondata(const vmu_root_t *root, const vmu_dir_t *dir);

/** \brief  Given a previously-read directory, locate the EXTRA_BG.PVR file.

    \param  root            The VMU root block.
    \param  dir             The VMU directory.
    \return                 The index into the directory array on success, or
                            <0 on failure.
*/
ssize_t vmufs_dir_extrabg(const vmu_root_t *root, const vmu_dir_t *dir);

/** \brief  Given a previously-read directory, add a new dirent to the dir.

    Another file with the same name should not exist (delete it first if it
    does). This function will not check for dupes!

    \param  root            The VMU root block.
    \param  dir             The VMU directory.
    \param  newdirent       The new entry to add.
    \return                 0 on success, or <0 on failure. */
int vmufs_dir_add(const vmu_root_t *root, vmu_dir_t *dir,
                  const vmu_dir_t *newdirent);

/** \brief  Given a previously-read directory, return the number of dirents
            available for new files.

    \param  root            The VMU root block.
    \param  dir             The directory in question.
    \return                 The number of entries available.
*/
size_t vmufs_dir_free(const vmu_root_t *root, const vmu_dir_t *dir);

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
int vmufs_file_read(maple_device_t *dev, const vmu_block_t *fat,
                    const vmu_dir_t *dirent, void *outbuf);

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
int vmufs_file_write(maple_device_t *dev, const vmu_root_t *root,
                     vmu_block_t *fat, vmu_dir_t *dir,
                     const vmu_dir_t *newdirent, const void *filebuf,
                     size_t size);

/** \brief  Given a previously-read FAT and directory, delete the named file.

    No changes are made to the VMU itself, just the in-memory structs.

    \param  root            The VMU root block.
    \param  fat             The FAT to be modified.
    \param  dir             The directory to be modified.
    \param  fn              The file name to be deleted.
    \retval 0               On success.
    \retval -1              If fn is not found.
*/
int vmufs_file_delete(const vmu_root_t *root, vmu_block_t *fat, vmu_dir_t *dir,
                      const char *fn);

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
int vmufs_readdir(maple_device_t *dev, vmu_dir_t **outbuf, size_t *outcnt);

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
int vmufs_read(maple_device_t *dev, const char *fn, void **outbuf,
               size_t *outsize);

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
int vmufs_read_dirent(maple_device_t *dev, const vmu_dir_t *dirent,
                      void **outbuf, size_t *outsize);

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
int vmufs_write(maple_device_t *dev, const char *fn, const void *inbuf,
                size_t insize, unsigned flags);

/** \brief  Delete a file from the VMU.

    \retval 0               On success.
    \retval -1              If the file is not found.
    \retval -2              On other failure.
*/
int vmufs_delete(maple_device_t *dev, const char *fn);

/** \brief  Return the number of user blocks free for file writing.

    You should check this number before attempting to write.

    \return                 The number of blocks free for writing.
*/
int vmufs_free_blocks(maple_device_t *dev);

/** \cond  */
/* Initialize vmufs. */
int vmufs_init(void);
/*  Shutdown vmufs. */
int vmufs_shutdown(void);
/** \endcond */

/** @} */

__END_DECLS

#endif  /* __DC_VMUFS_H */
