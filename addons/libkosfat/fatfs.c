/* KallistiOS ##version##

   fatfs.c
   Copyright (C) 2012, 2013, 2019 Lawrence Sebald
*/

#include <malloc.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "fatfs.h"
#include "bpb.h"
#include "fatinternal.h"

/* This is basically the same as bgrad_cache from fs_iso9660 */
static void make_mru(fat_fs_t *fs, fat_cache_t **cache, int block) {
    int i;
    fat_cache_t *tmp;

    /* Don't try it with the end block */
    if(block < 0 || block >= fs->cache_size - 1)
        return;

    /* Make a copy and scoot everything down */
    tmp = cache[block];

    for(i = block; i < fs->cache_size - 1; ++i) {
        cache[i] = cache[i + 1];
    }

    cache[fs->cache_size - 1] = tmp;
}

/* XXXX: This needs locking! */
uint8_t *fat_cluster_read(fat_fs_t *fs, uint32_t cl, int *err) {
    int i;
    uint8_t *rv;
    fat_cache_t **cache = fs->bcache;

    /* Look through the cache from the most recently used to the least recently
       used entry. */
    for(i = fs->cache_size - 1; i >= 0; --i) {
        if(cache[i]->block == cl && cache[i]->flags) {
            rv = cache[i]->data;
            make_mru(fs, cache, i);
            goto out;
        }
    }

    /* If we didn't get anything, did we end up with an invalid entry or do we
       need to boot someone out? */
    if(i < 0) {
        i = 0;

        /* Make sure that if the block is dirty, we write it back out. */
        if(cache[0]->flags & FAT_CACHE_FLAG_DIRTY) {
            if(fat_cluster_write_nc(fs, cache[0]->block, cache[0]->data)) {
                /* XXXX: Uh oh... */
                *err = EIO;
                return NULL;
            }
        }
    }

    /* Try to read the block in question. */
    if(fat_cluster_read_nc(fs, cl, cache[i]->data)) {
        *err = EIO;
        cache[i]->flags = 0;            /* Mark it as invalid... */
        return NULL;
    }

    cache[i]->block = cl;
    cache[i]->flags = FAT_CACHE_FLAG_VALID;
    rv = cache[i]->data;
    make_mru(fs, cache, i);

out:
    return rv;
}

uint8_t *fat_cluster_clear(fat_fs_t *fs, uint32_t cl, int *err) {
    int i;
    uint8_t *rv;
    fat_cache_t **cache = fs->bcache;

    /* Look through the cache from the most recently used to the least recently
       used entry. */
    for(i = fs->cache_size - 1; i >= 0; --i) {
        if(cache[i]->block == cl && cache[i]->flags) {
            rv = cache[i]->data;
            make_mru(fs, cache, i);
            goto out;
        }
    }

    /* If we didn't get anything, did we end up with an invalid entry or do we
       need to boot someone out? */
    if(i < 0) {
        i = 0;

        /* Make sure that if the block is dirty, we write it back out. */
        if(cache[0]->flags & FAT_CACHE_FLAG_DIRTY) {
            if(fat_cluster_write_nc(fs, cache[0]->block, cache[0]->data)) {
                /* XXXX: Uh oh... */
                *err = EIO;
                return NULL;
            }
        }
    }

    /* Don't bother reading the cluster from disk, since we're erasing it
       anyway... */
    cache[i]->block = cl;
    cache[i]->flags = FAT_CACHE_FLAG_VALID | FAT_CACHE_FLAG_DIRTY;
    rv = cache[i]->data;
    make_mru(fs, cache, i);

out:
    memset(rv, 0, fs->sb.bytes_per_sector * fs->sb.sectors_per_cluster);
    return rv;
}

int fat_cluster_read_nc(fat_fs_t *fs, uint32_t cluster, uint8_t *rv) {
    int fs_per_block = (int)fs->sb.sectors_per_cluster;

    if(fs_per_block < 0)
        /* This should never happen, as the cluster size must be at least
           as large as the sector size of the block device itself. */
        return -EINVAL;

    /* Are we reading a raw block (for FAT12/FAT16 root directory reading) or
       are we reading a normal cluster? */
    if(cluster & 0x80000000 && fs->sb.fs_type != FAT_FS_FAT32) {
        if(fs->dev->read_blocks(fs->dev, cluster & 0x7FFFFFFF, 1, rv))
            return -EIO;
    }
    else {
        if(fs->sb.num_clusters + 2 <= cluster || cluster < 2)
            return -EINVAL;

        cluster -= 2;

        if(fs->dev->read_blocks(fs->dev, cluster * fs_per_block +
                                fs->sb.first_data_block, fs_per_block, rv))
            return -EIO;
    }

    return 0;
}

int fat_cluster_write_nc(fat_fs_t *fs, uint32_t cluster, const uint8_t *blk) {
    int fs_per_block = (int)fs->sb.sectors_per_cluster;

    if(fs_per_block < 0)
        /* This should never happen, as the cluster size must be at least
           as large as the sector size of the block device itself. */
        return -EINVAL;

    /* Are we writing a raw block (for FAT12/FAT16 root directory updating) or
       are we writing a normal cluster? */
    if(cluster & 0x80000000 && fs->sb.fs_type != FAT_FS_FAT32) {
        if(fs->dev->write_blocks(fs->dev, cluster & 0x7FFFFFFF, 1, blk))
            return -EIO;
    }
    else {
        if(fs->sb.num_clusters + 2 <= cluster || cluster < 2)
            return -EINVAL;

        cluster -= 2;

        if(fs->dev->write_blocks(fs->dev, cluster * fs_per_block +
                                 fs->sb.first_data_block, fs_per_block, blk))
            return -EIO;
    }

    return 0;
}

int fat_cluster_mark_dirty(fat_fs_t *fs, uint32_t cluster) {
    int i;
    fat_cache_t **cache = fs->bcache;

    /* Look through the cache from the most recently used to the least recently
       used entry. */
    for(i = fs->cache_size - 1; i >= 0; --i) {
        if(cache[i]->block == cluster && cache[i]->flags) {
            cache[i]->flags |= FAT_CACHE_FLAG_DIRTY;
            make_mru(fs, cache, i);
            return 0;
        }
    }

    return -EINVAL;
}

int fat_cluster_cache_wb(fat_fs_t *fs) {
    int i, err;
    fat_cache_t **cache = fs->bcache;

    /* Don't even bother if we're mounted read-only. */
    if(!(fs->mnt_flags & FAT_MNT_FLAG_RW))
        return 0;

    for(i = fs->cache_size - 1; i >= 0; --i) {
        if(cache[i]->flags & FAT_CACHE_FLAG_DIRTY) {
            if((err = fat_cluster_write_nc(fs, cache[i]->block,
                                           cache[i]->data)))
                return err;

            cache[i]->flags &= ~FAT_CACHE_FLAG_DIRTY;
        }
    }

    return 0;
}

static inline uint32_t ilog2(uint32_t i) {
    i |= (i >> 1);
    i |= (i >> 2);
    i |= (i >> 4);
    i |= (i >> 8);
    i |= (i >> 16);

    return i - (i >> 1);
}

uint32_t fat_block_size(const fat_fs_t *fs)  {
    return fs->sb.bytes_per_sector;
}

uint32_t fat_log_block_size(const fat_fs_t *fs) {
    return ilog2(fs->sb.bytes_per_sector);
}

uint32_t fat_cluster_size(const fat_fs_t *fs) {
    return fs->sb.bytes_per_sector * fs->sb.sectors_per_cluster;
}

uint32_t fat_log_cluster_size(const fat_fs_t *fs) {
    return ilog2(fs->sb.bytes_per_sector * fs->sb.sectors_per_cluster);
}

uint32_t fat_blocks_per_cluster(const fat_fs_t *fs) {
    return fs->sb.sectors_per_cluster;
}

int fat_fs_type(const fat_fs_t *fs) {
    return (int)fs->sb.fs_type;
}

uint32_t fat_rootdir_length(const fat_fs_t *fs) {
    if(fs->sb.fs_type == FAT_FS_FAT32)
        return -1;
    return fs->sb.root_dir;
}

fat_fs_t *fat_fs_init(kos_blockdev_t *bd, uint32_t flags) {
    return fat_fs_init_ex(bd, flags, FAT_CACHE_BLOCKS, FAT_FCACHE_BLOCKS);
}

fat_fs_t *fat_fs_init_ex(kos_blockdev_t *bd, uint32_t flags, int cache_sz,
                         int fcache_sz) {
    fat_fs_t *rv;
    int j;
    int block_size, cluster_size;

    if(bd->init(bd)) {
        return NULL;
    }

    if(!(rv = (fat_fs_t *)malloc(sizeof(fat_fs_t)))) {
        bd->shutdown(bd);
        return NULL;
    }

    rv->dev = bd;
    rv->mnt_flags = flags & FAT_MNT_VALID_FLAGS_MASK;

    if(rv->mnt_flags != flags) {
        dbglog(DBG_WARNING, "fat_fs_init: unknown mount flags: %08" PRIx32
               "\n", flags);
        dbglog(DBG_WARNING, "             mounting read-only\n");
        rv->mnt_flags = 0;
    }

    /* Read in the boot block */
    if(fat_read_boot(&rv->sb, bd)) {
        free(rv);
        bd->shutdown(bd);
        return NULL;
    }

#ifdef FAT_DEBUG
    fat_print_superblock(&rv->sb);
#endif

    block_size = rv->sb.bytes_per_sector;
    cluster_size = rv->sb.bytes_per_sector * rv->sb.sectors_per_cluster;

    /* Make space for the block cache. */
    if(!(rv->bcache = (fat_cache_t **)malloc(sizeof(fat_cache_t *) *
                                             cache_sz))) {
        free(rv);
        bd->shutdown(bd);
        return NULL;
    }

    for(j = 0; j < cache_sz; ++j) {
        if(!(rv->bcache[j] = (fat_cache_t *)malloc(sizeof(fat_cache_t)))) {
            goto out_cache;
        }
    }

    for(j = 0; j < cache_sz; ++j) {
        if(!(rv->bcache[j]->data = (uint8_t *)memalign(32, cluster_size))) {
            goto out_bcache;
        }

        rv->bcache[j]->flags = 0;
    }

    rv->cache_size = cache_sz;

    /* Make space for the FAT block cache. */
    if(!(rv->fcache = (fat_cache_t **)malloc(sizeof(fat_cache_t *) *
                                             fcache_sz))) {
        goto out_bcache;
    }

    for(j = 0; j < fcache_sz; ++j) {
        if(!(rv->fcache[j] = (fat_cache_t *)malloc(sizeof(fat_cache_t)))) {
            goto out_fcache;
        }
    }

    for(j = 0; j < fcache_sz; ++j) {
        if(!(rv->fcache[j]->data = (uint8_t *)memalign(32, block_size))) {
            goto out_fcache2;
        }

        rv->fcache[j]->flags = 0;
    }

    rv->fcache_size = fcache_sz;
    return rv;

out_fcache2:
    for(; j >= 0; --j) {
        free(rv->fcache[j]->data);
    }

    j = fcache_sz - 1;

out_fcache:
    for(; j >= 0; --j) {
        free(rv->fcache[j]);
    }

    free(rv->fcache);

out_bcache:
    for(; j >= 0; --j) {
        free(rv->bcache[j]->data);
    }

    j = cache_sz - 1;
out_cache:
    for(; j >= 0; --j) {
        free(rv->bcache[j]);
    }

    free(rv->bcache);
    free(rv);
    bd->shutdown(bd);
    return NULL;
}

int fat_fs_sync(fat_fs_t *fs) {
    int rv, frv = 0;

    /* Don't even bother if we're mounted read-only. */
    if(!(fs->mnt_flags & FAT_MNT_FLAG_RW))
        return 0;

    /* Do a write-back on the block cache, which should take care of all
       the writes for regular data blocks... */
    if((rv = fat_cluster_cache_wb(fs))) {
        dbglog(DBG_ERROR, "fat_fs_sync: Error writing back the block cache: "
               "%s.\n", strerror(-rv));
        errno = -rv;
        frv = -1;
    }

    /* Do a write-back on the FAT cache now... */
    if((rv = fat_fatblock_cache_wb(fs))) {
        dbglog(DBG_ERROR, "fat_fs_sync: Error writing back the FAT cache: "
               "%s.\n", strerror(-rv));
        errno = -rv;
        frv = -2;
    }

    /* Write the FSinfo sector out... */
    if((rv = fat_write_fsinfo(fs))) {
        dbglog(DBG_ERROR, "fat_fs_sync: Error writing FSinfo sector: %s\n",
               strerror(-rv));
        errno = -rv;
        frv = -3;
    }

    return frv;
}

void fat_fs_shutdown(fat_fs_t *fs) {
    int i;

    /* Sync the filesystem back to the block device, if needed. */
    fat_fs_sync(fs);

    for(i = 0; i < fs->cache_size; ++i) {
        free(fs->bcache[i]->data);
        free(fs->bcache[i]);
    }

    free(fs->bcache);

    for(i = 0; i < fs->fcache_size; ++i) {
        free(fs->fcache[i]->data);
        free(fs->fcache[i]);
    }

    fs->dev->shutdown(fs->dev);
    free(fs);
}
