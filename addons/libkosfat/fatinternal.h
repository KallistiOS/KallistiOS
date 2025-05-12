/* KallistiOS ##version##

   fatinternal.h
   Copyright (C) 2012, 2013, 2019 Lawrence Sebald
*/

#ifndef __FAT_FATINTERNAL_H
#define __FAT_FATINTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include "bpb.h"

#define FAT_CACHE_FLAG_VALID    1
#define FAT_CACHE_FLAG_DIRTY    2

typedef struct fat_cache {
    uint32_t flags;
    uint32_t block;
    uint8_t *data;
} fat_cache_t;

struct fatfs_struct {
    kos_blockdev_t *dev;
    fat_superblock_t sb;

    fat_cache_t **bcache;
    int cache_size;

    fat_cache_t **fcache;
    int fcache_size;

    uint32_t flags;
    uint32_t mnt_flags;
};

/* The BPB/FSinfo blocks need to be written back to the block device... */
#define FAT_FS_FLAG_SB_DIRTY   1

#ifdef FAT_NOT_IN_KOS
    #include <stdio.h>
    #define DBG_DEBUG 0
    #define DBG_KDEBUG 0
    #define DBG_WARNING 0
    #define DBG_ERROR 0

    #define dbglog(lvl, ...) printf(__VA_ARGS__)
#else
    #include <kos/dbglog.h>
#endif

#endif /* !__FAT_FATINTERNAL_H */
