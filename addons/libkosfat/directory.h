/* KallistiOS ##version##

   directory.h
   Copyright (C) 2019 Lawrence Sebald
*/

#ifndef __FAT_DIRECTORY_H
#define __FAT_DIRECTORY_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

#include "fatfs.h"

typedef struct fat_dentry {
    uint8_t name[11];
    uint8_t attr;
    uint8_t reserved;
    uint8_t ctenth;
    uint16_t ctime;
    uint16_t cdate;
    uint16_t adate;
    uint16_t cluster_high;
    uint16_t mtime;
    uint16_t mdate;
    uint16_t cluster_low;
    uint32_t size;
} __packed fat_dentry_t;

#define FAT_ENTRY_EOD           0x00
#define FAT_ENTRY_FREE          0xE5

#define FAT_ATTR_READ_ONLY      0x01
#define FAT_ATTR_HIDDEN         0x02
#define FAT_ATTR_SYSTEM         0x04
#define FAT_ATTR_VOLUME_ID      0x08
#define FAT_ATTR_DIRECTORY      0x10
#define FAT_ATTR_ARCHIVE        0x20
#define FAT_ATTR_LONG_NAME      0x0F

#define FAT_ATTR_LONG_NAME_MASK 0x3F

#define FAT_IS_LONG_NAME(ent) (((ent)->attr & FAT_ATTR_LONG_NAME_MASK) == \
                                FAT_ATTR_LONG_NAME)

typedef struct fat_longname {
    uint8_t order;
    uint8_t name1[10];
    uint8_t attr;
    uint8_t type;
    uint8_t checksum;
    uint8_t name2[12];
    uint16_t cluster_low;
    uint8_t name3[4];
} __packed fat_longname_t;

#define FAT_ORDER_LAST  0x40

int fat_find_dentry(fat_fs_t *fs, const char *fn, fat_dentry_t *rv,
                    uint32_t *rcl, uint32_t *roff, uint32_t *rlcl,
                    uint32_t *rloff);
int fat_find_child(fat_fs_t *fs, const char *fn, fat_dentry_t *parent,
                   fat_dentry_t *rv, uint32_t *rcl, uint32_t *roff,
                   uint32_t *rlcl, uint32_t *rloff);
int fat_erase_dentry(fat_fs_t *fs, uint32_t cl, uint32_t off, uint32_t lcl,
                     uint32_t loff);
int fat_is_dir_empty(fat_fs_t *fs, uint32_t cluster);
int fat_add_dentry(fat_fs_t *fs, const char *fn, fat_dentry_t *parent,
                   uint8_t attr, uint32_t cluster, uint32_t *rcl,
                   uint32_t *roff, uint32_t *rlcl, uint32_t *rloff);
void fat_add_raw_dentry(fat_dentry_t *dent, const char shortname[11],
                        uint8_t attr, uint32_t cluster);
int fat_get_dentry(fat_fs_t *fs, uint32_t cl, uint32_t off, fat_dentry_t *rv);
int fat_update_dentry(fat_fs_t *fs, fat_dentry_t *ent, uint32_t cluster,
                      uint32_t off);
void fat_update_mtime(fat_dentry_t *ent);

#ifdef FAT_DEBUG
void fat_dentry_print(const fat_dentry_t *ent);
#endif

__END_DECLS

#endif /* !__FAT_DIRECTORY_H */
