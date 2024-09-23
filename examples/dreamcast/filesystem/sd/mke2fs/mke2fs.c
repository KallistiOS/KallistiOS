/* KallistiOS ##version##

   mke2fs.c
   Copyright (C) 2013 Lawrence Sebald

   This example shows how to format a SD card with a new ext2 filesystem using
   pretty much no functionality from libkosext2fs (other than the definitions in
   the headers). No functions in the library itself are called (hence the
   library isn't linked in).

   At some point I'll probably move some of this functionality into libkosext2fs
   so that there's less manual work to be done, but for the time being, this
   gets the job done.
*/

#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>

#ifdef _arch_dreamcast
#include <kos/dbgio.h>
#include <kos/blockdev.h>

#include <arch/arch.h>

#include <dc/sd.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#endif

#include "ext2fs.h"
#include "block.h"
#include "inode.h"
#include "superblock.h"
#include "utils.h"

#define KiB 1024LLU
#define MiB (KiB * 1024LLU)
#define GiB (MiB * 1024LLU)

/* We don't make any blocks more than 4KiB in size, so this works as a full-size
   block buffer quite nicely. */
static uint8_t block[4096] __attribute__((aligned(4)));

static ext2_bg_desc_t *bg_descs;
static int bg_count;
static uint32_t *rsvd_inodes, *rsvd_blocks;

/* <= 128MiB -> 1024 byte blocks
   <= 4GiB -> 2048 byte blocks
   > 4GiB -> 4096 byte blocks

   While libkosext2fs should handle larger block sizes than 4096 bytes, Linux on
   most platforms will not, so we don't go above it.

   Note that these ranges are somewhat arbitrary, but work out nicely. */
static inline uint32_t pick_ext2_bs(uint64_t total_size) {
    if(total_size > 4 * GiB)
        return 4096;
    else if(total_size > 128 * MiB)
        return 2048;
    else
        return 1024;
}

static inline int sb_tst(int group, int root) {
    for(;;) {
        if(group == 1)
            return 1;
        if(group % root)
            return 0;
        group /= root;
    }
}

static inline int has_superblock(int group) {
    if(group == 0 || sb_tst(group, 3) || sb_tst(group, 5) || sb_tst(group, 7))
        return 1;
    return 0;
}

static void __attribute__((__noreturn__)) exit_with_error(const char *err) {
#ifdef _arch_dreamcast
    maple_device_t *dev;
    cont_state_t *state;

    printf("%s\n\nPress any button to exit.\n", err);

    for(;;) {
        dev = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

        if(dev) {
            state = (cont_state_t *)maple_dev_status(dev);

            if(state)   {
                if(state->buttons)
                    arch_exit();
            }
        }
    }
#else
    printf("%s\n", err);
    exit(EXIT_FAILURE);
#endif
}

static int write_fs_block(ext2_superblock_t *sb, kos_blockdev_t *bd,
                          uint32_t block_num, const uint8_t *buf) {
    int fs_per_block = 10 + sb->s_log_block_size - bd->l_block_size;

    if(fs_per_block < 0)
        /* This should never happen, as the ext2 block size must be at least
           as large as the sector size of the block device itself. */
        return -EINVAL;

    if(sb->s_blocks_count <= block_num)
        return -EINVAL;

    if(bd->write_blocks(bd, block_num << fs_per_block, 1 << fs_per_block, buf))
        return -EIO;

    return 0;
}

static int read_fs_block(ext2_superblock_t *sb, kos_blockdev_t *bd,
                         uint32_t block_num, uint8_t *buf) {
    int fs_per_block = 10 + sb->s_log_block_size - bd->l_block_size;

    if(fs_per_block < 0)
        /* This should never happen, as the ext2 block size must be at least
           as large as the sector size of the block device itself. */
        return -EINVAL;

    if(sb->s_blocks_count <= block_num)
        return -EINVAL;

    if(bd->read_blocks(bd, block_num << fs_per_block, 1 << fs_per_block, buf))
        return -EIO;

    return 0;
}

static int read_inode_block(ext2_superblock_t *sb, kos_blockdev_t *bd,
                            uint32_t inode_num, uint8_t *buf,
                            ext2_inode_t **rino, uint32_t *rblk) {
    uint32_t bg, index, blk;
    uint16_t ino_sz;
    uint32_t block_size = 1024 << sb->s_log_block_size;
    int rv;

    if(sb->s_rev_level >= EXT2_DYNAMIC_REV)
        ino_sz = sb->s_inode_size;
    else
        ino_sz = 128;
    
    bg = (inode_num - 1) / sb->s_inodes_per_group;
    index = (inode_num - 1) % sb->s_inodes_per_group;

    blk = index / (block_size / ino_sz);
    index %= (block_size / ino_sz);
    *rblk = blk + bg_descs[bg].bg_inode_table;

    if((rv = read_fs_block(sb, bd, *rblk, buf)))
        return rv;

    *rino = (ext2_inode_t *)(buf + index * ino_sz);

    return 0;
}

static int write_superblock(ext2_superblock_t *bsb, kos_blockdev_t *bd,
                            uint32_t bg) {
    uint8_t *buf;
    ext2_superblock_t *sb;
    uint32_t blk, nblks;
    int fs_per_block = 10 + bsb->s_log_block_size - bd->l_block_size;
    int rv;

    /* Allocate enough space for one filesystem block. */
    if(!(buf = (uint8_t *)malloc(1024 << bsb->s_log_block_size)))
        return -ENOMEM;

    sb = (ext2_superblock_t *)buf;
    memset(buf, 0, 1024 << bsb->s_log_block_size);

    /* If we're working with the first block group, we need to offset within
       the block, potentially. */
    if(!bg) {
        if(bd->l_block_size > 10) {
            /* Read what's there already, in case we have a boot block or some
               other nonsense. */
            if(bd->read_blocks(bd, 0, 1, buf))
                return -EIO;

            /* Fix the pointer. */
            sb = (ext2_superblock_t *)(buf + 1024);

            /* Clear out anything after the superblock */
            if(bd->l_block_size > 11)
                memset(buf + 2048, 0, (1 << bd->l_block_size) - 2048);

            blk = 0;
            nblks = 1;
        }
        else {
            nblks = blk = 1024 >> bd->l_block_size;
        }
    }
    else {
        blk = (bg * bsb->s_blocks_per_group + bsb->s_first_data_block) <<
            (fs_per_block);
        nblks = 1 << fs_per_block;
    }

    /* Copy in the superblock */
    memcpy(sb, bsb, sizeof(ext2_superblock_t));

    /* Fix things up, depending on the revision of the filesystem. */
    if(bsb->s_rev_level >= EXT2_DYNAMIC_REV)
        /* Write the block group number. */
        sb->s_block_group_nr = (uint16_t)bg;
    else
        /* Clear everything that's not in rev0 out. */
        memset(&sb->s_first_ino, 0, 176);

    printf("Writing superblock for group %" PRIu32 " @ %" PRIu32 "\n", bg,
           blk >> fs_per_block);

    /* Write the data. */
    rv = bd->write_blocks(bd, blk, nblks, buf);

    /* Clean up, we're done. */
    free(buf);
    return rv;
}

static int write_bg_descs(ext2_superblock_t *sb, kos_blockdev_t *bd,
                          uint32_t bg) {
    uint32_t blk, nblks, bg_per_block, i;
    uint32_t block_size = (1024 << sb->s_log_block_size);
    uint8_t *buf;
    int rv = 0;

    /* The block group descriptors appear right after the superblock (backup) */
    blk = bg * sb->s_blocks_per_group + sb->s_first_data_block + 1;

    /* Figure out how big each superblock (or backup thereof) is */
    bg_per_block = block_size / sizeof(ext2_bg_desc_t);
    nblks = bg_count / bg_per_block;

    if(bg_count % bg_per_block)
        ++nblks;

    if(!(buf = (uint8_t *)malloc(block_size * nblks)))
        return -ENOMEM;

    memset(buf, 0, block_size * nblks);
    memcpy(buf, bg_descs, bg_count * sizeof(ext2_bg_desc_t));

    printf("Writing block group descriptors for group %" PRIu32 " @ %" PRIu32
           " (%" PRIu32 " block(s))\n", bg, blk, nblks);

    /* Write them */
    for(i = 0; i < nblks && !rv; ++i) {
        rv = write_fs_block(sb, bd, blk + i, buf + (block_size * i));
    }

    free(buf);
    return rv;
}

static int write_superblocks(ext2_superblock_t *sb, kos_blockdev_t *bd) {
    int i;

    printf("Writing superblocks\n");

    if(sb->s_rev_level < EXT2_DYNAMIC_REV ||
       !(sb->s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER)) {
        for(i = 0; i < bg_count; ++i) {
            if(write_superblock(sb, bd, i))
                exit_with_error("Error writing superblock\n");

            if(write_bg_descs(sb, bd, i))
                exit_with_error("Error writing block group descriptors\n");
        }
    }
    else {
        if(write_superblock(sb, bd, 0))
            exit_with_error("Error writing superblock\n");

        if(write_bg_descs(sb, bd, 0))
            exit_with_error("Error writing block group descriptors\n");

        for(i = 1; i < bg_count; ++i) {
            if(has_superblock(i)) {
                if(write_superblock(sb, bd, i))
                    exit_with_error("Error writing superblock\n");

                if(write_bg_descs(sb, bd, i))
                    exit_with_error("Error writing block group descriptors\n");
            }
        }
    }
        
    return -ENOSYS;
}

static int write_blank_inode_tables(ext2_superblock_t *sb, kos_blockdev_t *bd) {
    int i, rv;
    uint32_t j, blk;

    memset(block, 0, 1024 << sb->s_log_block_size);

    for(i = 0; i < bg_count; ++i) {
        blk = i * sb->s_blocks_per_group + rsvd_blocks[i] +
            sb->s_first_data_block;
        printf("Writing inode tables for block group %d\n"
               "\t%" PRIu32 " blocks (%" PRIu16 " inodes), start @ block %"
               PRIu32 "\n", i, rsvd_inodes[i], bg_descs[i].bg_free_inodes_count,
               blk);
        bg_descs[i].bg_inode_table = blk;

        for(j = 0; j < rsvd_inodes[i]; ++j) {
            if((rv = write_fs_block(sb, bd, blk++, block)))
                exit_with_error("Error writing inode tables!\n");
        }
    }

    return 0;
}

static int create_bg_descs(ext2_superblock_t *sb) {
    uint32_t bc = sb->s_blocks_count - sb->s_first_data_block;
    int odd_count = 0, i;

    /* Figure out how many block groups we'll have. */
    bg_count = bc / sb->s_blocks_per_group;
    if(bc % sb->s_blocks_per_group) {
        odd_count = 1;
        ++bg_count;
    }

    /* Allocate space for them */
    if(!(bg_descs = (ext2_bg_desc_t *)malloc(sizeof(ext2_bg_desc_t) *
                                             bg_count)))
        return -ENOMEM;

    if(!(rsvd_blocks = (uint32_t *)malloc(sizeof(uint32_t) * bg_count))) {
        free(bg_descs);
        return -ENOMEM;
    }

    if(!(rsvd_inodes = (uint32_t *)malloc(sizeof(uint32_t) * bg_count))) {
        free(rsvd_blocks);
        free(bg_descs);
        return -ENOMEM;
    }

    memset(bg_descs, 0, sizeof(ext2_bg_desc_t) * bg_count);
    memset(rsvd_blocks, 0, sizeof(uint32_t) * bg_count);
    memset(rsvd_inodes, 0, sizeof(uint32_t) * bg_count);

    sb->s_inodes_per_group = ((sb->s_inodes_count / bg_count) + 7) & ~7;
    sb->s_inodes_count = sb->s_inodes_per_group * bg_count;
    sb->s_free_inodes_count = sb->s_inodes_count - 11;

    /* Set up what we know for sure, we'll get the rest later. */
    for(i = 0; i < bg_count - odd_count; ++i) {
        bg_descs[i].bg_free_blocks_count = sb->s_blocks_per_group;
        bg_descs[i].bg_free_inodes_count = sb->s_inodes_per_group;
    }

    /* Handle the last group specially... */
    if(odd_count) {
        bg_descs[i].bg_free_blocks_count = sb->s_blocks_count -
            (sb->s_blocks_per_group * (bg_count - 1)) -
            sb->s_first_data_block;
        bg_descs[i].bg_free_inodes_count = sb->s_inodes_per_group;
    }

    return 0;
}

static int reserve_blocks(ext2_superblock_t *sb) {
    int i;
    uint32_t bc = sb->s_blocks_count - sb->s_first_data_block;
    uint32_t block_size = (1024 << sb->s_log_block_size);
    uint32_t bg_per_block, sb_blocks, in_per_block, inode_blocks;
    uint32_t total_reserved = 0;

    /* Figure out how big each superblock (or backup thereof) is */
    bg_per_block = block_size / sizeof(ext2_bg_desc_t);
    sb_blocks = 1 + (bg_count / bg_per_block);

    if(bg_count % bg_per_block)
        ++sb_blocks;

    /* Figure out how many blocks we have to reserve beyond that in each
       block group for inodes. */
    in_per_block = block_size / sizeof(ext2_inode_t);
    inode_blocks = sb->s_inodes_per_group / in_per_block;

    if(sb->s_inodes_per_group % in_per_block)
        ++inode_blocks;

    /* Make sure we have sufficient space in the last block group and that we
       aren't going to have to readjust things... */
    if(bc % sb->s_blocks_per_group) {
        /* Make sure there's actually enough blocks in the last block group to
           make things work properly... */
        if(bg_descs[bg_count - 1].bg_free_blocks_count <=
           sb_blocks + inode_blocks + 32) {
            printf("Dropping last block group due to insufficient space!\n");
            printf("This lowers the filesystem size by %" PRIu16 " blocks\n",
                   bg_descs[bg_count - 1].bg_free_blocks_count);
            sb->s_blocks_count -= bg_descs[bg_count - 1].bg_free_blocks_count;
            sb->s_free_blocks_count -= bg_descs[bg_count - 1].bg_free_blocks_count;
            --bg_count;

            sb->s_inodes_per_group = ((sb->s_inodes_count / bg_count) + 7) & ~7;
            sb->s_inodes_count = sb->s_inodes_per_group * bg_count;
            sb->s_free_inodes_count = sb->s_inodes_count - 11;
            inode_blocks = sb->s_inodes_per_group / in_per_block;

            if(sb->s_inodes_per_group % in_per_block)
                ++inode_blocks;
        }
    }

    if(sb->s_rev_level < EXT2_DYNAMIC_REV ||
       !(sb->s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER)) {
        for(i = 0; i < bg_count; ++i) {
            /* Each block group has a superblock backup (and its luggage),
               an inode table, and 2 blocks for bitmaps */
            rsvd_blocks[i] = sb_blocks + 2;
            rsvd_inodes[i] = inode_blocks;

            bg_descs[i].bg_free_blocks_count -= rsvd_blocks[i] + rsvd_inodes[i];
            bg_descs[i].bg_free_inodes_count = sb->s_inodes_per_group;
            total_reserved += rsvd_blocks[i] + rsvd_inodes[i];
        }
    }
    else {
        rsvd_blocks[0] = sb_blocks + 2;
        rsvd_inodes[0] = inode_blocks;
        bg_descs[0].bg_free_blocks_count -= rsvd_blocks[0] + rsvd_inodes[0];
        bg_descs[0].bg_free_inodes_count = sb->s_inodes_per_group;
        total_reserved += rsvd_blocks[0] + rsvd_inodes[0];

        for(i = 1; i < bg_count; ++i) {
            /* Some groups have a superblock backup (and its luggage) */
            if(has_superblock(i))
                rsvd_blocks[i] = sb_blocks;

            /* All groups have an inode table and 2 blocks for bitmaps */
            rsvd_blocks[i] += 2;
            rsvd_inodes[i] = inode_blocks;

            bg_descs[i].bg_free_blocks_count -= rsvd_blocks[i] + rsvd_inodes[i];
            bg_descs[i].bg_free_inodes_count = sb->s_inodes_per_group;
            total_reserved += rsvd_blocks[i] + rsvd_inodes[i];
        }
    }

    /* Print out where the backup superblocks are. */
    if(bg_count > 1) {
        printf("Superblock backups stored on blocks:\n\t");
        printf("%" PRIu32, sb->s_blocks_per_group +
               sb->s_first_data_block);

        if(sb->s_rev_level < EXT2_DYNAMIC_REV ||
           !(sb->s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER)) {
            for(i = 2; i < bg_count; ++i) {
                printf(", %" PRIu32, i * sb->s_blocks_per_group +
                       sb->s_first_data_block);
            }
        }
        else {
            for(i = 2; i < bg_count; ++i) {
                if(has_superblock(i))
                    printf(", %" PRIu32, i * sb->s_blocks_per_group +
                           sb->s_first_data_block);
            }
        }
    }
    else {
        printf("FS has only one block group, no superblock backups!\n");
    }

    printf("\n");

    printf("%" PRIu32 " total reserved blocks (superblocks, inodes, bitmaps)\n",
           total_reserved);
    sb->s_free_blocks_count -= total_reserved;
    printf("%" PRIu32 " blocks left afterwards\n", sb->s_free_blocks_count);

    return 0;
}

static int write_root_dir(ext2_superblock_t *sb, kos_blockdev_t *bd) {
    ext2_dirent_t *ent = (ext2_dirent_t *)block;
    uint8_t ft = 0;
    uint32_t bs = 1024 << sb->s_log_block_size;
    uint32_t blk, ino_blk;
    int rv, i;
    ext2_inode_t *ino;
    int fs_per_block = 10 + sb->s_log_block_size - bd->l_block_size;
    time_t now = time(NULL);

    if(sb->s_rev_level >= EXT2_DYNAMIC_REV &&
       (sb->s_feature_incompat & EXT2_FEATURE_INCOMPAT_FILETYPE))
        ft = EXT2_FT_DIR;

    memset(block, 0, bs);

    /* Fill in "." */
    ent->inode = EXT2_ROOT_INO;
    ent->rec_len = 12;
    ent->name_len = 1;
    ent->file_type = ft;
    ent->name[0] = '.';
    ent->name[1] = ent->name[2] = ent->name[3] = '\0';

    /* Fill in ".." */
    ent = (ext2_dirent_t *)(block + 12);
    ent->inode = EXT2_ROOT_INO;
    ent->rec_len = 12;
    ent->name_len = 2;
    ent->file_type = ft;
    ent->name[0] = ent->name[1] = '.';
    ent->name[2] = ent->name[3] = '\0';

    /* Fill in "lost+found" */
    ent = (ext2_dirent_t *)(block + 24);
    ent->inode = 11;
    ent->rec_len = bs - 24;
    ent->name_len = 10;
    ent->file_type = ft;
    strncpy((char *)ent->name, "lost+found", 12);

    /* Update the stuff in the block group descriptor. */
    bg_descs[0].bg_used_dirs_count += 2;
    blk = rsvd_blocks[0] + rsvd_inodes[0] + sb->s_first_data_block;
    rsvd_blocks[0] += 3;
    bg_descs[0].bg_free_blocks_count -= 3;
    sb->s_free_blocks_count -= 3;

    printf("Writing root directory @ %" PRIu32 "\n", blk);

    /* Write the directory to the fs */
    if((rv = write_fs_block(sb, bd, blk, block)))
        exit_with_error("Error writing root directory\n");

    /* Deal with lost+found (which is always at least two blocks) */
    memset(ent, 0, 20);
    ent = (ext2_dirent_t *)block;
    ent->inode = 11;

    ent = (ext2_dirent_t *)(block + 12);
    ent->rec_len = bs - 12;
    ent->inode = EXT2_ROOT_INO;

    printf("Writing lost+found directory @ %" PRIu32 "\n", blk + 1);

    if((rv = write_fs_block(sb, bd, blk + 1, block)))
        exit_with_error("Error writing lost+found\n");

    memset(block, 0, bs);

    ent = (ext2_dirent_t *)block;
    ent->rec_len = bs;

    if((rv = write_fs_block(sb, bd, blk + 2, block)))
        exit_with_error("Error writing lost+found\n");

    /* Fill in the inode for the root directory */
    if((rv = read_inode_block(sb, bd, EXT2_ROOT_INO, block, &ino, &ino_blk)))
        exit_with_error("Cannot read root inode\n");

    ino->i_mode = EXT2_S_IFDIR | EXT2_S_IRUSR | EXT2_S_IWUSR | EXT2_S_IXUSR |
        EXT2_S_IRGRP | EXT2_S_IXGRP | EXT2_S_IROTH | EXT2_S_IXOTH;
    ino->i_uid = 0;
    ino->i_size = bs;
    ino->i_atime = ino->i_ctime = ino->i_mtime = now;
    ino->i_dtime = 0;
    ino->i_gid = 0;
    ino->i_links_count = 3;
    ino->i_blocks = (1 << fs_per_block);
    ino->i_flags = 0;
    ino->i_osd1 = 0;
    ino->i_block[0] = blk;
    for(i = 1; i < 15; ++i) ino->i_block[i] = 0;
    ino->i_generation = 0;
    ino->i_file_acl = ino->i_dir_acl = 0;
    ino->i_faddr = 0;
    ino->i_osd2.l_i_frag = ino->i_osd2.l_i_fsize = 0;
    ino->i_osd2.reserved = 0;
    ino->i_osd2.l_i_uid_high = ino->i_osd2.l_i_gid_high = 0;
    ino->i_osd2.reserved2 = 0;

    if((rv = write_fs_block(sb, bd, ino_blk, block)))
        exit_with_error("Cannot write root inode\n");

    /* Fill in the inode for lost+found */
    if((rv = read_inode_block(sb, bd, 11, block, &ino, &ino_blk)))
        exit_with_error("Cannot read lost+found inode\n");

    ino->i_mode = EXT2_S_IFDIR | EXT2_S_IRUSR | EXT2_S_IWUSR | EXT2_S_IXUSR |
        EXT2_S_IRGRP | EXT2_S_IXGRP | EXT2_S_IROTH | EXT2_S_IXOTH;
    ino->i_uid = 0;
    ino->i_size = bs << 1;
    ino->i_atime = ino->i_ctime = ino->i_mtime = now;
    ino->i_dtime = 0;
    ino->i_gid = 0;
    ino->i_links_count = 2;
    ino->i_blocks = (2 << fs_per_block);
    ino->i_flags = 0;
    ino->i_osd1 = 0;
    ino->i_block[0] = blk + 1;
    ino->i_block[1] = blk + 2;
    for(i = 2; i < 15; ++i) ino->i_block[i] = 0;
    ino->i_generation = 0;
    ino->i_file_acl = ino->i_dir_acl = 0;
    ino->i_faddr = 0;
    ino->i_osd2.l_i_frag = ino->i_osd2.l_i_fsize = 0;
    ino->i_osd2.reserved = 0;
    ino->i_osd2.l_i_uid_high = ino->i_osd2.l_i_gid_high = 0;
    ino->i_osd2.reserved2 = 0;

    if((rv = write_fs_block(sb, bd, ino_blk, block)))
        exit_with_error("Cannot write lost+found inode\n");

    return 0;
}

static int write_bitmaps(ext2_superblock_t *sb, kos_blockdev_t *bd) {
    int i;
    uint32_t block_size = 1024 << sb->s_log_block_size;
    uint32_t bg_per_block, sb_blocks, j, blk, first_inode = 11;
    uint32_t last_grp_blks;

    /* Figure out how big each superblock (or backup thereof) is */
    bg_per_block = block_size / sizeof(ext2_bg_desc_t);
    sb_blocks = 1 + (bg_count / bg_per_block);

    if(bg_count % bg_per_block)
        ++sb_blocks;

    /* Go through each block group and write the block bitmap. */
    for(i = 0; i < bg_count - 1; ++i) {
        memset(block, 0, block_size);

        /* Set the bits for all used blocks */
        for(j = 0; j < rsvd_blocks[i] + rsvd_inodes[i]; ++j) {
            ext2_bit_set((uint32_t *)block, j);
        }

        blk = i * sb->s_blocks_per_group + sb->s_first_data_block;
        if(sb->s_rev_level < EXT2_DYNAMIC_REV ||
           !(sb->s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER) ||
           has_superblock(i))
            blk += sb_blocks;

        bg_descs[i].bg_block_bitmap = blk;
        printf("Writing block bitmap for group %d @ %" PRIu32 "\n", i, blk);

        if(write_fs_block(sb, bd, blk, block))
            exit_with_error("Error writing bitmap\n");
    }

    /* We might have padding bits in the last bitmap... */
    memset(block, 0, block_size);

    /* Set the bits for all used blocks */
    for(j = 0; j < rsvd_blocks[i] + rsvd_inodes[i]; ++j) {
        ext2_bit_set((uint32_t *)block, j);
    }

    /* Figure out how many padding bits we have and set them. */
    last_grp_blks = (sb->s_blocks_count - sb->s_first_data_block) %
        sb->s_blocks_per_group;

    if(last_grp_blks) {
        for(j = sb->s_blocks_per_group - 1; j >= last_grp_blks; --j) {
            ext2_bit_set((uint32_t *)block, j);
        }
    }

    blk = i * sb->s_blocks_per_group + sb->s_first_data_block;
    if(sb->s_rev_level < EXT2_DYNAMIC_REV ||
       !(sb->s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER) ||
       has_superblock(i))
        blk += sb_blocks;

    bg_descs[i].bg_block_bitmap = blk;
    printf("Writing block bitmap for group %d @ %" PRIu32 "\n", i, blk);

    if(write_fs_block(sb, bd, blk, block))
        exit_with_error("Error writing bitmap\n");

    /* We should only have inodes in group 0, so we only have to write anything
       but 0 bits to that one. */
    memset(block, 0, block_size);

    if(sb->s_rev_level >= EXT2_DYNAMIC_REV)
        first_inode = sb->s_first_ino;

    /* Set the reserved bits (inodes 1-11) */
    for(j = 0; j < first_inode; ++j) {
        ext2_bit_set((uint32_t *)block, j);
    }

    blk = sb->s_first_data_block + 1 + sb_blocks;
    bg_descs[0].bg_inode_bitmap = blk;

    printf("Writing inode bitmap for group 0 @ %" PRIu32 "\n", blk);

    if(write_fs_block(sb, bd, blk, block))
        exit_with_error("Error writing bitmap\n");

    memset(block, 0, block_size);

    for(i = 1; i < bg_count; ++i) {
        blk = i * sb->s_blocks_per_group + sb->s_first_data_block + 1;
        if(sb->s_rev_level < EXT2_DYNAMIC_REV ||
           !(sb->s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER) ||
           has_superblock(i))
            blk += sb_blocks;

        bg_descs[i].bg_inode_bitmap = blk;
        printf("Writing inode bitmap for group %d @ %" PRIu32 "\n", i, blk);

        if(write_fs_block(sb, bd, blk, block))
            exit_with_error("Error writing bitmap\n");
    }

    return 0;
}

/* For testing outside of KOS... */
#ifndef _arch_dreamcast
static int blockdev_dummy(kos_blockdev_t *d) {
    (void)d;
    return 0;
}

static int blockdev_shutdown(kos_blockdev_t *d) {
    FILE *fp = (FILE *)d->dev_data;

    fclose(fp);
    return 0;
}

static int blockdev_read(kos_blockdev_t *d, uint32_t block, size_t count,
                         void *buf) {
    FILE *fp = (FILE *)d->dev_data;
    int fd = fileno(fp);
    ssize_t rv;

    rv = pread(fd, buf, count << d->l_block_size, block << d->l_block_size);
    return (rv > 0) ? 0 : -1;
}

static int blockdev_write(kos_blockdev_t *d, uint32_t block, size_t count,
                   const void *buf) {
    FILE *fp = (FILE *)d->dev_data;
    int fd = fileno(fp);
    ssize_t rv;

    rv = pwrite(fd, buf, count << d->l_block_size, block << d->l_block_size);
    return (rv > 0) ? 0 : -1;
}

static uint32_t blockdev_count(kos_blockdev_t *d) {
    FILE *fp = (FILE *)d->dev_data;
    off_t len;

    fseeko(fp, 0, SEEK_END);
    len = ftello(fp);
    fseeko(fp, 0, SEEK_SET);

    return (uint32_t)(len >> d->l_block_size);
}

static kos_blockdev_t the_bd = {
    NULL,
    9,

    &blockdev_dummy,
    &blockdev_shutdown,

    &blockdev_read,
    &blockdev_write,
    &blockdev_count
};

static int sd_init(void) {
    return 0;
}

static void sd_shutdown(void) {
}
#endif

int main(int argc, char *argv[]) {
    kos_blockdev_t sd_dev;
    uint8_t partition_type;
    uint32_t block_count, inode_count;
    uint64_t partition_size;
    uint32_t fs_blocksz;
    ext2_superblock_t sb;
    time_t now = time(NULL);
    int i;

    /* Use framebuffer debug output. */
#ifdef _arch_dreamcast
    dbgio_dev_select("fb");
#endif

    srand(now);

    if(sd_init()) {
        exit_with_error("Could not initialize the SD card.\n"
                        "Please make sure that you have an SD card\n"
                        "adapter plugged in and an SD card inserted.");        
    }

#ifdef _arch_dreamcast
    (void)argc;
    (void)argv;

    /* Grab the block device for the first partition on the SD card. Note that
       you must have the SD card formatted with an MBR partitioning scheme. */
    if(sd_blockdev_for_partition(0, &sd_dev, &partition_type)) {
        sd_shutdown();
        exit_with_error("Could not find the first partition on the SD card!\n");
    }

    /* Read the MBR so we can change the partition type if needed. */
    if(sd_read_blocks(0, 1, block)) {
        sd_shutdown();
        exit_with_error("Cannot read the MBR from the SD card!\n");
    }

    /* If it isn't already set to 0x83 (Linux), set it to 0x83. */
    if(block[0x01BE + 4] != 0x83) {
        printf("Partition type 0x%02x will be replaced by 0x83\n",
               block[0x01BE + 4]);
        block[0x01BE + 4] = 0x83;

        if(sd_write_blocks(0, 1, block)) {
            sd_shutdown();
            exit_with_error("Cannot write MBR back to the SD card!\n");
        }
    }
#else
    (void)partition_type;

    if(argc != 2)
        exit_with_error("Must supply an image filename!\n");

    /* Whee... random scopes for fun and profit! */
    {
        FILE *fp = fopen(argv[1], "r+b");
        if(!fp) {
            exit_with_error("Cannot open filesystem image file\n");
        }

        the_bd.dev_data = fp;
        sd_dev = the_bd;
    }
#endif
        

    /* Initialize the block device. */
    if(sd_dev.init(&sd_dev)) {
        sd_shutdown();
        exit_with_error("Could not initialize the SD block device!\n");
    }

    /* Figure out how large the partition is, so we know how big to make our
       filesystem blocks. */
    block_count = sd_dev.count_blocks(&sd_dev);
    partition_size = (uint64_t)(1 << sd_dev.l_block_size) * block_count;

    printf("%" PRIu32 " raw blocks in partition (%d bytes each)\n", block_count,
           1 << sd_dev.l_block_size);
    printf("Detected partition size of %" PRIu64 " bytes\n", partition_size);

    fs_blocksz = pick_ext2_bs(partition_size);

    if(fs_blocksz < (uint32_t)(1 << sd_dev.l_block_size)) {
        fs_blocksz = 1 << sd_dev.l_block_size;

        if(fs_blocksz > 4 * KiB) {
            sd_dev.shutdown(&sd_dev);
            sd_shutdown();
            exit_with_error("Cowardly refusing to make a filesystem with a\n"
                            "block size of > 4096 bytes.");
        }
    }

    if(partition_size / fs_blocksz < 64) {
        sd_dev.shutdown(&sd_dev);
        sd_shutdown();
        exit_with_error("Cowardly refusing to make a filesystem with\n"
                        "less than 64 blocks.");
    }

    printf("Will create ext2fs with %" PRIu32 " byte blocks\n", fs_blocksz);

    /* Figure out how many blocks and inodes we'll make. Arbitrarily take inode
       count to be block_count / 4. */
    block_count = partition_size / fs_blocksz;
    inode_count = ((block_count / 4) + 7) & ~7;
    printf("Initial params: %" PRIu32 " FS blocks; %" PRIu32 " inodes\n",
           block_count, inode_count);

    /* Start filling in our superblock. */
    memset(&sb, 0, sizeof(ext2_superblock_t));
    sb.s_inodes_count = inode_count;
    sb.s_blocks_count = block_count;
    sb.s_r_blocks_count = 0;
    sb.s_free_blocks_count = block_count - (fs_blocksz == 1024 ? 1 : 0);
    sb.s_free_inodes_count = inode_count - 11;
    sb.s_first_data_block = (fs_blocksz == 1024 ? 1 : 0);
    sb.s_log_block_size = fs_blocksz >> 11;
    sb.s_log_frag_size = fs_blocksz >> 11;
    sb.s_blocks_per_group = fs_blocksz * 8;
    sb.s_frags_per_group = fs_blocksz * 8;
    sb.s_inodes_per_group = fs_blocksz * 2;
    sb.s_mtime = sb.s_wtime = now;
    sb.s_mnt_count = 0;
    sb.s_max_mnt_count = 64;                    /* Picked arbitrarily. */
    sb.s_magic = 0xEF53;
    sb.s_state = EXT2_VALID_FS;
    sb.s_errors = EXT2_ERRORS_RO;
    sb.s_minor_rev_level = 0;
    sb.s_lastcheck = now;
    sb.s_checkinterval = 180 * 60 * 60 * 24;    /* 180 days */
    sb.s_creator_os = EXT2_OS_LINUX;            /* Yep... We're Linux. ;-) */
    sb.s_rev_level = EXT2_DYNAMIC_REV;
    sb.s_def_resuid = 0;
    sb.s_def_resgid = 0;
    sb.s_first_ino = 11;
    sb.s_inode_size = 128;
    sb.s_block_group_nr = 0;
    sb.s_feature_compat = EXT2_FEATURE_COMPAT_EXT_ATTR;
    sb.s_feature_incompat = EXT2_FEATURE_INCOMPAT_FILETYPE;
    sb.s_feature_ro_compat = EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER |
        EXT2_FEATURE_RO_COMPAT_LARGE_FILE;

    /* Make a nice v4 UUID... */
    for(i = 0; i < 16; ++i) {
        sb.s_uuid[i] = (uint8_t)rand();
    }

    sb.s_uuid[6] = 0x40 | (sb.s_uuid[6] & 0x0F);
    sb.s_uuid[8] = 0x80 | (sb.s_uuid[8] & 0x3F);

    strncpy((char *)sb.s_volume_name, "KallistiOS", 16);

    /* Now that we have a superblock, start on the block group descriptors. */
    printf("Creating block group descriptors\n");
    if(create_bg_descs(&sb)) {
        sd_dev.shutdown(&sd_dev);
        sd_shutdown();
        exit_with_error("Cannot create block group descriptors!\n");
    }

    printf("Counted %d block groups\n", bg_count);

    reserve_blocks(&sb);

    printf("Final filesystem params: %" PRIu32 " blocks, %" PRIu32 " inodes\n",
           sb.s_blocks_count, sb.s_inodes_count);
    write_blank_inode_tables(&sb, &sd_dev);
    write_root_dir(&sb, &sd_dev);
    write_bitmaps(&sb, &sd_dev);
    bg_descs[0].bg_free_inodes_count -= 11;
    write_superblocks(&sb, &sd_dev);

    sd_dev.shutdown(&sd_dev);
    sd_shutdown();

    exit_with_error("Format complete.");
}
