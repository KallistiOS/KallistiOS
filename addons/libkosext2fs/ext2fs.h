/* KallistiOS ##version##

   ext2fs.h
   Copyright (C) 2012, 2013 Lawrence Sebald
*/

#ifndef __EXT2_EXT2FS_H
#define __EXT2_EXT2FS_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

#ifndef EXT2_NOT_IN_KOS
#include <kos/blockdev.h>
#endif

/* Tunable filesystem parameters. These must be set at compile time. */

/* Logarithm (base 2) of the maximum number of entries in the inode cache.
   Thus, the inode cache will take up (2^n) * (128 + k) bytes of total space in
   RAM (where k is the overhead for accounting information -- 32 bytes in KOS).
   Set this to a larger number to ensure you can have a lot of files open at
   once. Also, note that this is a global cache, regardless of how many
   filesystems you actually have mounted. */
#define EXT2_LOG_MAX_INODES     7

/* Logarithm (base 2) of the number of head nodes in the inode hash table. The
   larger this is, the more entries the root array for the inode hash table will
   have, and thus the lower the probability of collisions in the table. Each
   entry in the array is a list, so collisions aren't fatal or anything like
   that. Setting this to something larger than EXT2_LOG_MAX_INODES is somewhat
   silly. I suggest either setting it the same or slightly less than the above
   constant. */
#define EXT2_LOG_INODE_HASH     (EXT2_LOG_MAX_INODES - 2)

/* Size of the block cache, in filesystem blocks. When reading from the
   filesystem, all data is read in block-sized units. The size of a block can
   generally range from 1024 bytes to 4096 bytes, and is dependent on the
   parameters the filesystem was formatted with. Increasing this value should
   ensure that more accesses can be handled by the cache, but also increases the
   latency at which data is written back to the block device itself. Setting
   this to 32 should work well enough, but if you have more memory to spare,
   feel free to set it larger.

   Note that this is a default value for filesystems initialized/mounted with
   ext2_fs_init(). If you wish to specify your own value that differs from this
   one, you can do so with the ext2_fs_init_ex() function.
*/
#define EXT2_CACHE_BLOCKS       32

/* End tunable filesystem parameters. */

/* Convenience stuff, for in case you want to use this outside of KOS. */
#ifdef EXT2_NOT_IN_KOS

typedef struct kos_blockdev {
    void *dev_data;
    uint32_t l_block_size;
    int (*init)(struct kos_blockdev *d);
    int (*shutdown)(struct kos_blockdev *d);
    int (*read_blocks)(const struct kos_blockdev *d, uint32_t block, size_t count,
                       void *buf);
    int (*write_blocks)(const struct kos_blockdev *d, uint32_t block, size_t count,
                        const void *buf);
    uint32_t (*count_blocks)(const struct kos_blockdev *d);
} kos_blockdev_t;

#ifndef SYMLOOP_MAX
#define SYMLOOP_MAX 16
#endif

#endif /* EXT2_NOT_IN_KOS */

/* Opaque ext2 filesystem type */
struct ext2fs_struct;
typedef struct ext2fs_struct ext2_fs_t;

/* Filesystem mount flags */
#define EXT2FS_MNT_FLAG_RO          0x00000000
#define EXT2FS_MNT_FLAG_RW          0x00000001

/* Valid flags mask */
#define EXT2FS_MNT_VALID_FLAGS_MASK 0x00000001

uint32_t ext2_block_size(const ext2_fs_t *fs);
uint32_t ext2_log_block_size(const ext2_fs_t *fs);

/* Initialize low-level structures (like the global inode cache). If you don't
   call this before calling ext2_fs_init(), it will be called for you before
   mounting the first filesystem. */
int ext2_init(void);

ext2_fs_t *ext2_fs_init(kos_blockdev_t *bd, uint32_t flags);
ext2_fs_t *ext2_fs_init_ex(kos_blockdev_t *bd, uint32_t flags, int cache_sz);
int ext2_fs_sync(ext2_fs_t *fs);
void ext2_fs_shutdown(ext2_fs_t *fs);

int ext2_block_read_nc(ext2_fs_t *fs, uint32_t block_num, uint8_t *rv);
uint8_t *ext2_block_read(ext2_fs_t *fs, uint32_t block_num, int *err);

int ext2_block_write_nc(ext2_fs_t *fs, uint32_t block_num, const uint8_t *blk);

int ext2_block_mark_dirty(ext2_fs_t *fs, uint32_t block_num);

/* Write-back all dirty blocks from the filesystem's cache. You probably want to
   call the corresponding inode function before this one. */
int ext2_block_cache_wb(ext2_fs_t *fs);

uint8_t *ext2_block_alloc(ext2_fs_t *fs, uint32_t bg, uint32_t *bn, int *err);

__END_DECLS

#endif /* !__EXT2_EXT2FS_H */
