/* KallistiOS ##version##

   fatfs.h
   Copyright (C) 2019 Lawrence Sebald
*/

#ifndef __FAT_FATFS_H
#define __FAT_FATFS_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

#ifndef FAT_NOT_IN_KOS
#include <kos/blockdev.h>
#endif

/* Tunable filesystem parameters. These must be set at compile time. */

/* Size of the cluster cache, in filesystem clusters. When reading data from the
   filesystem, all data is read in cluster-sized units. The size of a cluster
   can generally range from 1024 bytes to 65536 bytes, and is dependent on the
   parameters the filesystem was formatted with. Increasing this value should
   ensure that more accesses can be handled by the cache, but also increases the
   latency at which data is written back to the block device itself. Setting
   this to 8 should work well enough, but if you have more memory to spare,
   feel free to set it larger (just keep in mind your target cluster size!).
   For reference, a 16-cluster cache at 64k clusters would give a cache size
   of 1MiB (plus some overhead).

   Note that this is a default value for filesystems initialized/mounted with
   fat_fs_init(). If you wish to specify your own value that differs from this
   one, you can do so with the fat_fs_init_ex() function.
*/
#define FAT_CACHE_BLOCKS        8

/* Size of the FAT cache, in filesystem blocks. When reading the file allocation
   table, all data is read one block at a time. Generally, a block is 512 bytes
   in size (and much of the code in this library makes the assumption that this
   is the case). This value must be at least set to 2 in order to ensure that
   FAT12 support works in the library. The default value of 8 should work well
   enough, but feel free to increase it if you have more memory to spare.

   Just like FAT_CACHE_BLOCKS above, this is a default for filesystems
   initialized/mounted with fat_fs_init(). You may specify your own value at
   runtime by using the fat_fs_init_ex() function.
*/
#define FAT_FCACHE_BLOCKS       8

/* End tunable filesystem parameters. */

/* Convenience stuff, for in case you want to use this outside of KOS. */
#ifdef FAT_NOT_IN_KOS

typedef struct kos_blockdev {
    void *dev_data;
    uint32_t l_block_size;
    int (*init)(struct kos_blockdev *d);
    int (*shutdown)(struct kos_blockdev *d);
    int (*read_blocks)(const struct kos_blockdev *d, uint64_t block, size_t count,
                       void *buf);
    int (*write_blocks)(const struct kos_blockdev *d, uint64_t block, size_t count,
                        const void *buf);
    uint32_t (*count_blocks)(const struct kos_blockdev *d);
} kos_blockdev_t;
#endif /* FAT_NOT_IN_KOS */

/* Opaque ext2 filesystem type */
struct fatfs_struct;
typedef struct fatfs_struct fat_fs_t;

/* Filesystem mount flags */
#define FAT_MNT_FLAG_RO             0x00000000
#define FAT_MNT_FLAG_RW             0x00000001

/* Valid flags mask */
#define FAT_MNT_VALID_FLAGS_MASK    0x00000001

fat_fs_t *fat_fs_init(kos_blockdev_t *bd, uint32_t flags);
fat_fs_t *fat_fs_init_ex(kos_blockdev_t *bd, uint32_t flags, int cache_sz,
                         int fcache_sz);
int fat_fs_sync(fat_fs_t *fs);
void fat_fs_shutdown(fat_fs_t *fs);

int fat_cluster_read_nc(fat_fs_t *fs, uint32_t cluster, uint8_t *rv);
uint8_t *fat_cluster_read(fat_fs_t *fs, uint32_t cluster, int *err);
uint8_t *fat_cluster_clear(fat_fs_t *fs, uint32_t cl, int *err);

int fat_cluster_write_nc(fat_fs_t *fs, uint32_t cluster, const uint8_t *blk);

int fat_cluster_mark_dirty(fat_fs_t *fs, uint32_t cluster);

uint32_t fat_block_size(const fat_fs_t *fs);
uint32_t fat_log_block_size(const fat_fs_t *fs);
uint32_t fat_cluster_size(const fat_fs_t *fs);
uint32_t fat_log_cluster_size(const fat_fs_t *fs);
uint32_t fat_blocks_per_cluster(const fat_fs_t *fs);
uint32_t fat_rootdir_length(const fat_fs_t *fs);

#define FAT_FS_FAT12    0
#define FAT_FS_FAT16    1
#define FAT_FS_FAT32    2

int fat_fs_type(const fat_fs_t *fs);

/* Write-back all dirty blocks from the filesystem's cache. There's probably
   not many good reasons for you to call these two functions... The
   fat_fs_sync() function is a better idea. */
int fat_cluster_cache_wb(fat_fs_t *fs);
int fat_fatblock_cache_wb(fat_fs_t *fs);

#define FAT_FREE_CLUSTER    0x00000000
#define FAT_INVALID_CLUSTER 0xFFFFFFFF

#define FAT_EOC_FAT32       0x0FFFFFF8
#define FAT_EOC_FAT16       0xFFF8
#define FAT_EOC_FAT12       0x0FF8

uint32_t fat_read_fat(fat_fs_t *fs, uint32_t cl, int *err);
int fat_write_fat(fat_fs_t *fs, uint32_t cl, uint32_t val);
int fat_is_eof(fat_fs_t *fs, uint32_t cl);
uint32_t fat_allocate_cluster(fat_fs_t *fs, int *err);
int fat_erase_chain(fat_fs_t *fs, uint32_t cluster);

__END_DECLS

#endif /* !__FAT_FATFS_H */
