/* KallistiOS ##version##

   bpb.c
   Copyright (C) 2012, 2019 Lawrence Sebald
*/

#include <malloc.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include "fatfs.h"
#include "fatinternal.h"

static int fat_read_raw_boot(fat_bootblock_t *sb, kos_blockdev_t *bd) {
    if(bd->l_block_size > 9) {
        uint8_t *buf;

        if(!(buf = (uint8_t *)memalign(32, 1 << bd->l_block_size)))
            return -ENOMEM;

        if(bd->read_blocks(bd, 0, 1, buf))
            return -EIO;

        memcpy(sb, buf, 512);
        free(buf);
        return 0;
    }
    else {
        return bd->read_blocks(bd, 0, 512 >> bd->l_block_size, sb);
    }
}

static int fat_read_fsinfo(fat32_fsinfo_t *fsinfo, int s, kos_blockdev_t *bd) {
    if(bd->l_block_size > 9) {
        /* XXXX: Probably should handle this case at some point... */
        return -EIO;
    }
    else if(bd->l_block_size == 9) {
        /* This should generally be the case... */
        return bd->read_blocks(bd, s, 1, fsinfo);
    }
    else {
        /* This shouldn't happen. */
        return -EIO;
    }
}

static int fat_write_raw_fsinfo(fat_fs_t *fs, fat32_fsinfo_t *fsinfo, int s) {
    if(fs->dev->l_block_size > 9) {
        /* XXXX: Probably should handle this case at some point... */
        return -EIO;
    }
    else if(fs->dev->l_block_size == 9) {
        /* This should generally be the case... */
        return fs->dev->write_blocks(fs->dev, s, 1, fsinfo);
    }
    else {
        /* This shouldn't happen. */
        return -EIO;
    }
}

int fat_write_fsinfo(fat_fs_t *fs) {
    fat32_fsinfo_t fsinfo;
    int err;

    /* Don't let us write to the volume if we're on a read-only FS. */
    if(!(fs->mnt_flags & FAT_MNT_FLAG_RW))
        return -EROFS;

    /* Only FAT32 has this sector. */
    if(fs->sb.fs_type != FAT_FS_FAT32)
        return 0;

    /* Read the old value in... */
    if((err = fat_read_fsinfo(&fsinfo, fs->sb.fsinfo_sector, fs->dev)))
        return err;

    fsinfo.fsinfo_sig1 = FAT32_FSINFO_SIG1;
    fsinfo.fsinfo_sig2 = FAT32_FSINFO_SIG2;
    fsinfo.free_clusters = fs->sb.free_clusters;
    fsinfo.last_alloc_cluster = fs->sb.last_alloc_cluster;
    fsinfo.fsinfo_sig3 = FAT32_FSINFO_SIG3;

    /* Write the first copy of the fsinfo sector... */
    if((err = fat_write_raw_fsinfo(fs, &fsinfo, fs->sb.fsinfo_sector)))
        return err;

    /* Write the backup copy if one exists. */
    if(fs->sb.backup_bpb)
        err = fat_write_raw_fsinfo(fs, &fsinfo, fs->sb.backup_bpb + 1);

    return err;
}

static int fat_parse_boot(fat_bootblock_t *bb, fat_superblock_t *sb) {
    uint32_t rds, fsz, ts, ds, bps, rs, nc, fds;

    /* Do work needed to determine what type of filesystem we have... */
    rds = bb->bpb.root_dir_entries[0] | (bb->bpb.root_dir_entries[1] << 8);
    bps = bb->bpb.bytes_per_sector[0] | (bb->bpb.bytes_per_sector[1] << 8);
    rds = ((rds << 5) + (bps - 1)) / bps;

    fsz = bb->bpb.fat_size[0] | (bb->bpb.fat_size[1] << 8);
    if(!fsz)
        fsz = bb->ebpb.fat32.fat_size[0] | (bb->ebpb.fat32.fat_size[1] << 8) |
            (bb->ebpb.fat32.fat_size[2] << 16) |
            (bb->ebpb.fat32.fat_size[3] << 24);

    ts = bb->bpb.num_sectors16[0] | (bb->bpb.num_sectors16[1] << 8);
    if(!ts)
        ts = bb->bpb.num_sectors32[0] | (bb->bpb.num_sectors32[1] << 8) |
            (bb->bpb.num_sectors32[2] << 16) | (bb->bpb.num_sectors32[3] << 24);

    rs = bb->bpb.reserved_sectors[0] | (bb->bpb.reserved_sectors[1] << 8);
    fds = rs + (bb->bpb.num_fats * fsz) + rds;
    ds = ts - fds;

    nc = ds / bb->bpb.sectors_per_cluster;

    /* Fill in stuff in the superblock structure. */
    sb->num_sectors = ts;
    sb->fat_size = fsz;
    sb->bytes_per_sector = bps;
    sb->reserved_sectors = rs;
    sb->num_clusters = nc;
    sb->num_fats = bb->bpb.num_fats;
    sb->sectors_per_cluster = bb->bpb.sectors_per_cluster;
    sb->first_data_block = fds;

    if(nc <= FAT_MAX_FAT12_CLUSTERS)
        sb->fs_type = FAT_FS_FAT12;
    else if(nc <= FAT_MAX_FAT16_CLUSTERS)
        sb->fs_type = FAT_FS_FAT16;
    else
        sb->fs_type = FAT_FS_FAT32;

    if(rds)
        sb->root_dir = bb->bpb.root_dir_entries[0] |
            (bb->bpb.root_dir_entries[1] << 8);
    else
        sb->root_dir = bb->ebpb.fat32.rootdir_cluster[0] |
            (bb->ebpb.fat32.rootdir_cluster[1] << 8) |
            (bb->ebpb.fat32.rootdir_cluster[2] << 16) |
            (bb->ebpb.fat32.rootdir_cluster[3] << 24);

    if(sb->fs_type == FAT_FS_FAT32) {
        /* Make sure that the filesystem is sane... */
        if(bb->ebpb.fat32.fs_version[0] || bb->ebpb.fat32.fs_version[1])
            return -EINVAL;

        sb->fsinfo_sector = bb->ebpb.fat32.fsinfo_sector[0] |
            bb->ebpb.fat32.fsinfo_sector[1];

        sb->backup_bpb = bb->ebpb.fat32.backup_bpb[0] |
            bb->ebpb.fat32.backup_bpb[1];

        if(bb->ebpb.fat32.ext_boot_sig == 0x28 ||
           bb->ebpb.fat32.ext_boot_sig == 0x29)
            memcpy(sb->volume_id, bb->ebpb.fat32.volume_id, 4);

        if(bb->ebpb.fat32.ext_boot_sig == 0x29)
            memcpy(sb->volume_label, bb->ebpb.fat32.volume_label, 11);
    }
    else {
        if(bb->ebpb.fat16.ext_boot_sig == 0x28 ||
           bb->ebpb.fat16.ext_boot_sig == 0x29)
            memcpy(sb->volume_id, bb->ebpb.fat16.volume_id, 4);

        if(bb->ebpb.fat16.ext_boot_sig == 0x29)
            memcpy(sb->volume_label, bb->ebpb.fat16.volume_label, 11);
    }

    return 0;
}

int fat_read_boot(fat_superblock_t *sb, kos_blockdev_t *bd) {
    int rv;
    fat_bootblock_t bb;
    fat32_fsinfo_t fsinfo;

    memset(&bb, 0, sizeof(fat_bootblock_t));
    memset(sb, 0, sizeof(fat_superblock_t));

    if((rv = fat_read_raw_boot(&bb, bd)) < 0) {
        return rv;
    }

    /* Make sure it looks sane... */
    if(bb.ebpb.fat16.boot_sig[0] != 0x55 || bb.ebpb.fat16.boot_sig[1] != 0xAA) {
        return -EINVAL;
    }

    if((rv = fat_parse_boot(&bb, sb)) < 0) {
        return rv;
    }

    /* Make sure our sector sizes match up... */
    if(sb->bytes_per_sector != (1 << bd->l_block_size)) {
        return -EINVAL;
    }

    /* If we have an fsinfo sector, read it. */
    if(sb->fsinfo_sector) {
        memset(&fsinfo, 0, sizeof(fat32_fsinfo_t));

        if((rv = fat_read_fsinfo(&fsinfo, sb->fsinfo_sector, bd)) < 0) {
            return rv;
        }

        /* Check it for sanity... */
        if(fsinfo.fsinfo_sig1 != FAT32_FSINFO_SIG1 ||
           fsinfo.fsinfo_sig2 != FAT32_FSINFO_SIG2 ||
           fsinfo.fsinfo_sig3 != FAT32_FSINFO_SIG3) {
            dbglog(DBG_KDEBUG, "Potentially invalid FSinfo sector: "
                   "%08" PRIx32 " %08" PRIx32 " %08" PRIx32 "\n",
                   fsinfo.fsinfo_sig1, fsinfo.fsinfo_sig2,
                   fsinfo.fsinfo_sig3);
        }
        else {
            /* Parse out what we care about... */
            sb->free_clusters = fsinfo.free_clusters;
            sb->last_alloc_cluster = fsinfo.last_alloc_cluster;
        }
    }
    else {
        sb->last_alloc_cluster = 2;
    }

    return 0;
}

#ifdef FAT_DEBUG
static const char *fs_types[] = { "FAT12", "FAT16", "FAT32" };
void fat_print_superblock(const fat_superblock_t *sb) {
    char label[12];

    dbglog(DBG_KDEBUG, "FAT Superblock:\n");
    dbglog(DBG_KDEBUG, "Filesystem type: %s\n", fs_types[sb->fs_type]);
    dbglog(DBG_KDEBUG, "Sector Count: %" PRIu32 "\n", sb->num_sectors);
    dbglog(DBG_KDEBUG, "FAT Size: %" PRIu32 "\n", sb->fat_size);
    dbglog(DBG_KDEBUG, "Number of FAT copies: %" PRIu8 "\n", sb->num_fats);
    dbglog(DBG_KDEBUG, "Sectors per cluster: %" PRIu16 "\n",
           sb->sectors_per_cluster);
    dbglog(DBG_KDEBUG, "Bytes per sector: %" PRIu16 "\n", sb->bytes_per_sector);
    dbglog(DBG_KDEBUG, "Reserved sectors: %" PRIu16 "\n", sb->reserved_sectors);
    dbglog(DBG_KDEBUG, "First data block: %" PRIu32 "\n", sb->first_data_block);
    dbglog(DBG_KDEBUG, "Volume ID: %02" PRIx8 "%02" PRIx8 "-%02" PRIx8 "%02"
           PRIx8 "\n", sb->volume_id[3], sb->volume_id[2], sb->volume_id[1],
           sb->volume_id[0]);
    memcpy(label, sb->volume_label, 11);
    label[11] = 0;
    dbglog(DBG_KDEBUG, "Volume Label: '%s'\n", label);

    if(sb->fs_type == FAT_FS_FAT32) {
        dbglog(DBG_KDEBUG, "Root directory cluster: %" PRIu32 "\n",
               sb->root_dir);
        dbglog(DBG_KDEBUG, "FSinfo Sector: %" PRIu16 "\n", sb->fsinfo_sector);
        dbglog(DBG_KDEBUG, "Backup BPB: %" PRIu16 "\n", sb->backup_bpb);
        dbglog(DBG_KDEBUG, "Free clusters: %" PRIu32 "\n",
               sb->free_clusters);
        dbglog(DBG_KDEBUG, "Last used cluster: %" PRIu32 "\n",
               sb->last_alloc_cluster);
    }
    else {
        dbglog(DBG_KDEBUG, "Root directory size: %" PRIu32 "\n", sb->root_dir);
    }
}
#endif /* FAT_DEBUG */
