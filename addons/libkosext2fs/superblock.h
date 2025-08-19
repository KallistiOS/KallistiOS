/* KallistiOS ##version##

   superblock.h
   Copyright (C) 2012 Lawrence Sebald
*/

#ifndef __EXT2_SUPERBLOCK_H
#define __EXT2_SUPERBLOCK_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

#ifndef EXT2_NOT_IN_KOS
#include <kos/blockdev.h>
#else
#include "ext2fs.h"
#endif

/** \brief  ext2fs Superblock structure.

    This structure represents the superblock of an ext2 filesystem. This applies
    both to the main superblock (always at byte 1024 of an ext2 filesystem) as
    well as any backup superblocks on the filesystem.

    The superblock of the filesystem stores various essential information to
    access the filesystem, including the number of blocks in the filesystem and
    the number of inodes.

    All multi-byte integers in the ext2 superblock are in little-endian byte
    order, which makes our lives a bit easier.

    Note that the names of the elements in this structure were taken from Dave
    Poirier's Second Extended File System documentation, which is available at
    http://www.nongnu.org/ext2-doc/ . The field names should match those that
    are used in the Linux kernel itself, so if you want better documentation of
    what is in here, either refer to Dave Poirier's document or the Linux kernel
    source code.

    \headerfile ext2/superblock.h
*/
typedef struct ext2_superblock {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;

    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t s_uuid[16];
    uint8_t s_volume_name[16];
    uint8_t s_last_mounted[64];
    uint32_t s_algo_bitmap;

    uint8_t s_prealloc_blocks;
    uint8_t s_prealloc_dir_blocks;
    uint16_t reserved0;

    uint8_t s_journal_uuid[16];
    uint32_t s_journal_inum;
    uint32_t s_journal_dev;
    uint32_t s_last_orphan;

    uint32_t s_hash_seed[4];
    uint8_t s_def_hash_version;
    uint8_t reserved1[3];

    uint32_t s_default_mount_options;
    uint32_t s_first_meta_bg;

    uint8_t unused[760];
} __packed ext2_superblock_t;

/* s_state values */
#define EXT2_VALID_FS   1
#define EXT2_ERROR_FS   2

/* s_errors values */
#define EXT2_ERRORS_CONTINUE    1
#define EXT2_ERRORS_RO          2
#define EXT2_ERRORS_PANIC       3

/* s_creator_os values */
#define EXT2_OS_LINUX   0
#define EXT2_OS_HURD    1
#define EXT2_OS_MASIX   2
#define EXT2_OS_FREEBSD 3
#define EXT2_OS_LITES   4

/* s_rev_level values */
#define EXT2_GOOD_OLD_REV   0
#define EXT2_DYNAMIC_REV    1

/* s_feature_compat values */
#define EXT2_FEATURE_COMPAT_DIR_PREALLOC    0x0001
#define EXT2_FEATURE_COMPAT_IMAGIC_INODES   0x0002
#define EXT2_FEATURE_COMPAT_HAS_JOURNAL     0x0004
#define EXT2_FEATURE_COMPAT_EXT_ATTR        0x0008
#define EXT2_FEATURE_COMPAT_RESIZE_INO      0x0010
#define EXT2_FEATURE_COMPAT_DIR_INDEX       0x0020

/* s_feature_incompat values */
#define EXT2_FEATURE_INCOMPAT_COMPRESSION   0x0001
#define EXT2_FEATURE_INCOMPAT_FILETYPE      0x0002
#define EXT2_FEATURE_INCOMPAT_RECOVER       0x0004
#define EXT2_FEATURE_INCOMPAT_JOURNAL_DEV   0x0008
#define EXT2_FEATURE_INCOMPAT_META_BG       0x0010

/* s_feature_ro_compat values */
#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE   0x0002
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR    0x0004

/* s_algo_bitmap values */
#define EXT2_LZV1_ALG       0x00000001
#define EXT2_LZRW3A_ALG     0x00000002
#define EXT2_GZIP_ALG       0x00000004
#define EXT2_BZIP_ALG       0x00000008
#define EXT2_LZO_ALG        0x00000010

/** \brief  Read the main superblock of an ext2 filesystem.

    This function reads the main superblock of an ext2 filesystem. The main
    superblock is the one that is stored in the first block of block group 0 of
    the filesystem (namely the one stored exactly 1024 bytes from the start of
    the filesystem).

    \param  sb          Storage for the incoming superblock.
    \param  bd          The block device to read from.
    \retval 0           On success.
    \retval -1          On failure. errno should be set as appropriate by the
                        block device or other failing function.
*/
int ext2_read_superblock(ext2_superblock_t *sb, kos_blockdev_t *bd);

/* Ugh... */
struct ext2fs_struct;

/** \brief  Write the superblock of an ext2 filesystem to either the main
            superblock or any of the backups.

    This function writes the superblock of an ext2 filesystem back to the block
    device. This function will write back either the main superblock (the one
    stored exactly 1024 bytes from the start of the filesystem) or any of the
    backups, depending on the block group specified.

    \param  fs          The filesystem to write to.
    \param  bg          Which block group to write-back to. To write to the main
                        superblock, specify 0.
    \retval 0           On success.
    \retval -1          On failure. errno should be set as appropriate by the
                        block device or other failing function. If this fails on
                        the main superblock, your filesystem is probably toast.
*/
int ext2_write_superblock(struct ext2fs_struct *fs, uint32_t bg);

#ifdef EXT2FS_DEBUG
void ext2_print_superblock(const ext2_superblock_t *sb);
#endif

__END_DECLS

#endif /* !__EXT2_SUPERBLOCK_H */
