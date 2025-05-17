/* KallistiOS ##version##

   ext2internal.h
   Copyright (C) 2012, 2013 Lawrence Sebald
*/

#include "block.h"
#include "superblock.h"

#ifndef EXT2_NOT_IN_KOS
#include <kos/blockdev.h>
#else
#include "ext2fs.h"
#endif

#ifndef __EXT2_EXT2INTERNAL_H
#define __EXT2_EXT2INTERNAL_H

#define EXT2_CACHE_FLAG_VALID   1
#define EXT2_CACHE_FLAG_DIRTY   2

typedef struct ext2_cache {
    uint32_t flags;
    uint32_t block;
    uint8_t *data;
} ext2_cache_t;

struct ext2fs_struct {
    kos_blockdev_t *dev;
    ext2_superblock_t sb;
    uint32_t block_size;

    uint32_t bg_count;
    ext2_bg_desc_t *bg;

    ext2_cache_t **bcache;
    int cache_size;

    uint32_t flags;
    uint32_t mnt_flags;
};

/* The superblock and/or block descriptors need to be written to the block
   device. */
#define EXT2_FS_FLAG_SB_DIRTY   1

#ifdef EXT2_NOT_IN_KOS
    #include <stdio.h>
    #define DBG_DEBUG 0
    #define DBG_KDEBUG 0
    #define DBG_WARNING 0
    #define DBG_ERROR 0

    #define dbglog(lvl, ...) printf(__VA_ARGS__)
#else
    #include <kos/dbglog.h>
#endif

#endif /* !__EXT2_EXT2INTERNAL_H */
