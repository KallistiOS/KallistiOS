/* KallistiOS ##version##

   bpb.h
   Copyright (C) 2019 Lawrence Sebald
*/

#ifndef __FAT_BPB_H
#define __FAT_BPB_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

typedef struct fat_bpb {
    uint8_t jmp[3];         /* 0xEB 0x?? 0x90 or 0xE9 0x?? 0x?? */
    uint8_t oem_name[8];
    uint8_t bytes_per_sector[2];
    uint8_t sectors_per_cluster;
    uint8_t reserved_sectors[2];
    uint8_t num_fats;
    uint8_t root_dir_entries[2];
    uint8_t num_sectors16[2];
    uint8_t media_code;
    uint8_t fat_size[2];
    uint8_t sectors_per_track[2];
    uint8_t num_heads[2];
    uint8_t hidden_sector_count[4];
    uint8_t num_sectors32[4];
} __attribute__((packed)) fat_bpb_t;

typedef struct fat16_ebpb {
    uint8_t drive_number;
    uint8_t reserved;
    uint8_t ext_boot_sig;
    uint8_t volume_id[4];
    uint8_t volume_label[11];
    uint8_t fs_type[8];
    uint8_t boot_code[448];
    uint8_t boot_sig[2];
} __attribute__((packed)) fat16_ebpb_t;

typedef struct fat32_ebpb {
    uint8_t fat_size[4];
    uint8_t flags[2];
    uint8_t fs_version[2];
    uint8_t rootdir_cluster[4];
    uint8_t fsinfo_sector[2];
    uint8_t backup_bpb[2];
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved2;
    uint8_t ext_boot_sig;
    uint8_t volume_id[4];
    uint8_t volume_label[11];
    uint8_t fs_type[8];
    uint8_t boot_code[420];
    uint8_t boot_sig[2];
} __attribute__((packed)) fat32_ebpb_t;

typedef struct fat_bootblock {
    fat_bpb_t bpb;
    union {
        fat16_ebpb_t fat16;
        fat32_ebpb_t fat32;
    } ebpb;
} __attribute__((packed,aligned(32))) fat_bootblock_t;

typedef struct fat32_fsinfo {
    uint32_t fsinfo_sig1;
    uint8_t reserved[480];
    uint32_t fsinfo_sig2;
    uint32_t free_clusters;
    uint32_t last_alloc_cluster;
    uint8_t reserved2[12];
    uint32_t fsinfo_sig3;
} __attribute__((packed,aligned(32))) fat32_fsinfo_t;

#define FAT32_FSINFO_SIG1 0x41615252
#define FAT32_FSINFO_SIG2 0x61417272
#define FAT32_FSINFO_SIG3 0xAA550000

typedef struct fat_superblock {
    uint32_t num_sectors;
    uint32_t fat_size;
    uint32_t root_dir;
    uint32_t num_clusters;
    uint32_t first_data_block;

    uint32_t free_clusters;
    uint32_t last_alloc_cluster;
    uint16_t fsinfo_sector;
    uint16_t backup_bpb;

    uint8_t volume_id[4];
    uint8_t volume_label[11];
    uint8_t fs_type;

    uint16_t bytes_per_sector;
    uint16_t reserved_sectors;
    uint8_t sectors_per_cluster;
    uint8_t num_fats;
} fat_superblock_t;

#define FAT_MAX_FAT12_CLUSTERS  4084
#define FAT_MAX_FAT16_CLUSTERS  65524

int fat_read_boot(fat_superblock_t *sb, kos_blockdev_t *bd);
int fat_write_fsinfo(fat_fs_t *fs);

#ifdef FAT_DEBUG
void fat_print_superblock(const fat_superblock_t *sb);
#endif

__END_DECLS

#endif /* !__FAT_BPB_H */
