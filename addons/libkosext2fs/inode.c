/* KallistiOS ##version##

   inode.c
   Copyright (C) 2012, 2013 Lawrence Sebald
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <limits.h>
#include <kos/limits.h>
#include <assert.h>
#include <sys/queue.h>
#include <inttypes.h>
#include <time.h>

#include "inode.h"
#include "utils.h"
#include "ext2fs.h"
#include "ext2internal.h"
#include "directory.h"

#ifdef __STRICT_ANSI__
/* These don't necessarily get prototyped in string.h in standard-compliant mode
   as they are extensions to the standard. Declaring them this way shouldn't
   hurt. */
char *strtok_r(char *, const char *, char **);
char *strdup(const char *);
#endif

#define MAX_INODES      (1 << EXT2_LOG_MAX_INODES)
#define INODE_HASH_SZ   (1 << EXT2_LOG_INODE_HASH)

#define INODE_FLAG_DIRTY    0x00000001

/* Internal inode storage structure. This is used for caching used inodes. */
static struct int_inode {
    /* Start with the on-disk inode itself to make the put() function easier.
       DO NOT MOVE THIS FROM THE BEGINNING OF THE STRUCTURE. */
    ext2_inode_t inode;

    /* Hash table entry -- used at all points after the first time an inode is
       placed into this entry. */
    LIST_ENTRY(int_inode) entry;

    /* Unused list entry -- used initially by all inodes, and later when the
       inode has had its reference counter decremented to 0. Note that when the
       second case there happens, the inode will still be in the inode hash
       table. That way, if we happen to pull it back up again later, we still
       can use the cached version and not have to re-read it from the block
       device. */
    TAILQ_ENTRY(int_inode) qentry;

    /* Flags for this inode. */
    uint32_t flags;

    /* Reference count for the inode. */
    uint32_t refcnt;

    /* What filesystem is this inode on? */
    ext2_fs_t *fs;

    /* What inode number is this? */
    uint32_t inode_num;
} inodes[MAX_INODES];

/* Head types */
LIST_HEAD(inode_list, int_inode);
TAILQ_HEAD(inode_queue, int_inode);

/* Tail queue of free/unused inodes. */
static struct inode_queue free_inodes;

/* Hash table of inodes in use. */
static struct inode_list inode_hash[INODE_HASH_SZ];

/* Forward declaration... */
static ext2_inode_t *ext2_inode_read(ext2_fs_t *fs, uint32_t inode_num);
static int ext2_inode_wb(struct int_inode *inode);

void ext2_inode_init(void) {
    int i;

    /* Initialize the hash table to its starting state */
    for(i = 0; i < INODE_HASH_SZ; ++i) {
        LIST_INIT(&inode_hash[i]);
    }

    /* Put all the inodes in the unused list */
    TAILQ_INIT(&free_inodes);

    for(i = 0; i < MAX_INODES; ++i) {
        inodes[i].flags = 0;
        inodes[i].inode_num = 0;
        inodes[i].refcnt = 0;
        TAILQ_INSERT_TAIL(&free_inodes, inodes + i, qentry);
    }
}

ext2_inode_t *ext2_inode_get(ext2_fs_t *fs, uint32_t inode_num, int *err) {
    int ent = inode_num & (INODE_HASH_SZ - 1);
    struct int_inode *i;
    ext2_inode_t *rinode;

    /* Figure out if this inode is already in the hash table. */
    LIST_FOREACH(i, &inode_hash[ent], entry) {
        if(i->fs == fs && i->inode_num == inode_num) {
            /* Increase the reference count, and see if it was free before. */
            if(!i->refcnt++) {
                /* It is in the free list. Remove it from the free list. */
                TAILQ_REMOVE(&free_inodes, i, qentry);
            }

#ifdef EXT2FS_DEBUG
            dbglog(DBG_KDEBUG, "ext2_inode_get: %" PRIu32 " (%" PRIu32
                   " refs)\n", inode_num, i->refcnt);
#endif
            return (ext2_inode_t *)i;
        }
    }

    /* Didn't find it... */
    if(!(i = TAILQ_FIRST(&free_inodes))) {
        /* Uh oh... No more free inodes... */
        *err = -ENFILE;
        return NULL;
    }

    /* Ok, at this point, we have a free inode, remove it from the free pool. */
    TAILQ_REMOVE(&free_inodes, i, qentry);

    /* Remove it from any old hash table lists it was in */
    if(i->inode_num)
        LIST_REMOVE(i, entry);

    i->refcnt = 1;
    i->inode_num = inode_num;
    i->fs = fs;

    /* Read the inode in from the block device. */
    if(!(rinode = ext2_inode_read(fs, inode_num))) {
        /* Hrm... what to do about that... */
        i->refcnt = 0;
        i->inode_num = 0;
        i->fs = NULL;
        TAILQ_INSERT_HEAD(&free_inodes, i, qentry);
        *err = -EIO;
        return NULL;
    }

    /* Add it to the hash table. */
    i->inode = *rinode;
    LIST_INSERT_HEAD(&inode_hash[ent], i, entry);

#ifdef EXT2FS_DEBUG
    dbglog(DBG_KDEBUG, "ext2_inode_get: %" PRIu32 " (%" PRIu32 " refs)\n",
           inode_num, i->refcnt);
#endif

    /* Ok... That should do it. */
    return &i->inode;
}

void ext2_inode_put(ext2_inode_t *inode) {
    struct int_inode *iinode = (struct int_inode *)inode;

    /* Make sure we're not trying anything really mean. */
    assert(iinode->refcnt != 0);

    /* Decrement the reference counter, and see if we've got the last one. */
    if(!--iinode->refcnt) {
        /* Write it back out to the block cache if it was dirty. */
        if(iinode->flags & INODE_FLAG_DIRTY)
            /* XXXX: Should probably make sure this succeeds... */
            ext2_inode_wb(iinode);

        /* We've gone and consumed the last reference, so put it on the free
           list at the end, in case we want to bring it back from the dead later
           on. */
        TAILQ_INSERT_TAIL(&free_inodes, iinode, qentry);
    }

#ifdef EXT2FS_DEBUG
    /* Technically not thread-safe, but then again, there's many bigger issues
       with thread-safety in this library than this. */
    dbglog(DBG_KDEBUG, "ext2_inode_put: %" PRIu32 " (%" PRIu32 " refs)\n",
           iinode->inode_num, iinode->refcnt);
#endif
}

void ext2_inode_retain(ext2_inode_t *inode) {
    struct int_inode *iinode = (struct int_inode *)inode;

    /* Make sure we're not trying anything really dumb. */
    assert(iinode->refcnt != 0);

    /* Increment the reference counter. */
    ++iinode->refcnt;

#ifdef EXT2FS_DEBUG
    /* Technically not thread-safe, but then again, there's many bigger issues
       with thread-safety in this library than this. */
    dbglog(DBG_KDEBUG, "ext2_inode_retain: %" PRIu32 " (%" PRIu32 " refs)\n",
           iinode->inode_num, iinode->refcnt);
#endif
}

void ext2_inode_mark_dirty(ext2_inode_t *inode) {
    struct int_inode *iinode = (struct int_inode *)inode;

    /* Make sure we're not trying anything really mean. */
    assert(iinode->refcnt != 0);

    iinode->flags |= INODE_FLAG_DIRTY;
}

static ext2_inode_t *ext2_inode_read(ext2_fs_t *fs, uint32_t inode_num) {
    uint32_t bg, index;
    uint8_t *buf;
    int in_per_block;
    uint32_t inode_block;
    int err;

    in_per_block = (fs->block_size) / fs->sb.s_inode_size;

    /* Figure out what block group and index within that group the inode in
       question is. */
    bg = (inode_num - 1) / fs->sb.s_inodes_per_group;
    index = (inode_num - 1) % fs->sb.s_inodes_per_group;

    if(inode_num > fs->sb.s_inodes_count)
        return NULL;

    if(!(buf = ext2_block_read(fs, fs->bg[bg].bg_inode_bitmap, &err)))
        return NULL;

    if(!ext2_bit_is_set((uint32_t *)buf, index))
        return NULL;

    /* Read the block containing the inode in.
       TODO: Should we check if the block is marked as in use? */
    inode_block = fs->bg[bg].bg_inode_table + (index / in_per_block);
    index %= in_per_block;

    if(!(buf = ext2_block_read(fs, inode_block, &err)))
        return NULL;

    /* Return the inode in question  */
    return (ext2_inode_t *)(buf + (index * fs->sb.s_inode_size));
}

static int ext2_inode_wb(struct int_inode *inode) {
    uint32_t bg, index;
    uint8_t *buf;
    int in_per_block, rv;
    uint32_t inode_block;
    ext2_fs_t *fs = inode->fs;

    /* Don't even bother if we're mounted read-only. */
    if(!(fs->mnt_flags & EXT2FS_MNT_FLAG_RW))
        return 0;

    in_per_block = (fs->block_size) / fs->sb.s_inode_size;

    /* Figure out what block group and index within that group the inode in
       question is. */
    bg = (inode->inode_num - 1) / fs->sb.s_inodes_per_group;
    index = (inode->inode_num - 1) % fs->sb.s_inodes_per_group;

    if(inode->inode_num > fs->sb.s_inodes_count)
        return -EINVAL;
    
    /* Read the block containing the inode in so that we can write the part that
       we need to it. */
    inode_block = fs->bg[bg].bg_inode_table + (index / in_per_block);
    index %= in_per_block;

    if(!(buf = ext2_block_read(fs, inode_block, &rv)))
        return rv;

    /* Write to the block and mark it as dirty so that it'll get flushed. */
    memcpy(buf + (index * fs->sb.s_inode_size), inode, sizeof(ext2_inode_t));
    rv = ext2_block_mark_dirty(fs, inode_block);

    /* Clear the dirty flag, if we wrote it out successfully. */
    if(!rv)
        inode->flags &= ~INODE_FLAG_DIRTY;

    return rv;
}

int ext2_inode_cache_wb(ext2_fs_t *fs) {
    int i, rv = 0;

    /* Don't even bother if we're mounted read-only. */
    if(!(fs->mnt_flags & EXT2FS_MNT_FLAG_RW))
        return 0;

    for(i = 0; i < MAX_INODES && !rv; ++i) {
        if(inodes[i].fs == fs && (inodes[i].flags & INODE_FLAG_DIRTY)) {
            rv = ext2_inode_wb(inodes + i);
        }
    }

    return rv;
}

ext2_inode_t *ext2_inode_alloc(ext2_fs_t *fs, uint32_t parent, int *err,
                               uint32_t *ninode) {
    uint8_t *buf;
    uint32_t index;
    struct int_inode *i;
    uint32_t bg;

    /* Don't even bother if we're mounted read-only. */
    if(!(fs->mnt_flags & EXT2FS_MNT_FLAG_RW)) {
        *err = EROFS;
        return NULL;
    }

    /* See if we have any free inodes at all... */
    if(!fs->sb.s_free_inodes_count) {
        *err = ENOSPC;
        return NULL;
    }

    /* Figure out what block group the parent is in and try that one first. */
    bg = (parent - 1) / fs->sb.s_inodes_per_group;

    /* See if we have any free inodes in the block group requested. */
    if(fs->bg[bg].bg_free_inodes_count) {
        if(!(buf = ext2_block_read(fs, fs->bg[bg].bg_inode_bitmap, err)))
            return NULL;

        index = ext2_bit_find_zero((uint32_t *)buf, 0,
                                   fs->sb.s_inodes_per_group - 1);
        if(index < fs->sb.s_inodes_per_group) {
            ext2_bit_set((uint32_t *)buf, index);
            ext2_block_mark_dirty(fs, fs->bg[bg].bg_inode_bitmap);
            --fs->bg[bg].bg_free_inodes_count;
            --fs->sb.s_free_inodes_count;
            fs->flags |= EXT2_FS_FLAG_SB_DIRTY;
            i = (struct int_inode *)ext2_inode_get(fs, index + bg *
                                                   fs->sb.s_inodes_per_group +
                                                   1, err);
            memset(i, 0, sizeof(ext2_inode_t));
            i->flags |= INODE_FLAG_DIRTY;
            *ninode = i->inode_num;
            return (ext2_inode_t *)i;
        }

        /* We shouldn't get here... But, just in case, fall through. We should
           probably log an error and tell the user to fsck though. */
        dbglog(DBG_WARNING, "ext2_inode_alloc: Block group %" PRIu32 " "
               "indicates that it has free inodes, but doesn't appear to. "
               "Please run fsck on this volume!\n", bg);
    }

    /* Couldn't find a free inode in the requested block group... Loop through
       all the block groups looking for a free inode. */
    for(bg = 0; bg < fs->bg_count; ++bg) {
        if(fs->bg[bg].bg_free_inodes_count) {
            if(!(buf = ext2_block_read(fs, fs->bg[bg].bg_inode_bitmap, err)))
                return NULL;

            index = ext2_bit_find_zero((uint32_t *)buf, 0,
                                       fs->sb.s_inodes_per_group - 1);
            if(index < fs->sb.s_inodes_per_group) {
                ext2_bit_set((uint32_t *)buf, index);
                ext2_block_mark_dirty(fs, fs->bg[bg].bg_inode_bitmap);
                --fs->bg[bg].bg_free_inodes_count;
                --fs->sb.s_free_inodes_count;
                fs->flags |= EXT2_FS_FLAG_SB_DIRTY;
                i = (struct int_inode *)ext2_inode_get(fs, index + bg *
                                                       fs->sb.s_inodes_per_group
                                                       + 1, err);
                memset(i, 0, sizeof(ext2_inode_t));
                i->flags |= INODE_FLAG_DIRTY;
                *ninode = i->inode_num;
                return (ext2_inode_t *)i;
            }

            /* We shouldn't get here... But, just in case, fall through. We
               should probably log an error and tell the user to fsck though. */
            dbglog(DBG_WARNING, "ext2_inode_alloc: Block group %" PRIu32 " "
                   "indicates that it has free inodes, but doesn't appear to. "
                   "Please run fsck on this volume!\n", bg);
        }
    }

    /* Uh oh... We went through everything and didn't find any. That means the
       data in the superblock is wrong. */
    dbglog(DBG_WARNING, "ext2_inode_alloc: Filesystem indicates that it has "
           "free inodes, but doesn't appear to. Please run fsck on this "
           "volume!\n");
    *err = ENOSPC;
    return NULL;
}

static inline int mark_block_free(ext2_fs_t *fs, uint32_t blk) {
    uint32_t bg, index;
    uint8_t *buf;
    int err;

    bg = (blk - fs->sb.s_first_data_block) / fs->sb.s_blocks_per_group;
    index = (blk - fs->sb.s_first_data_block) % fs->sb.s_blocks_per_group;

    if(!(buf = ext2_block_read(fs, fs->bg[bg].bg_block_bitmap, &err)))
        return -EIO;

    /* Mark the block as free in the bitmap and increase the counters. */
    ext2_bit_clear((uint32_t *)buf, index);
    ext2_block_mark_dirty(fs, fs->bg[bg].bg_block_bitmap);
    ++fs->bg[bg].bg_free_blocks_count;
    ++fs->sb.s_free_blocks_count;

    return 0;
}

static int free_ind_block(ext2_fs_t *fs, ext2_inode_t *inode, uint32_t iblk) {
    uint32_t blks_per_ind = fs->block_size >> 2;
    uint32_t i, blk;
    uint32_t *iblock;
    int rv;

    (void)inode;

    if(!iblk) {
        /* ????: This really shouldn't happen! */
        dbglog(DBG_ERROR, "ext2_inode_free_all: inode indicates use of block 0 "
               "for an indirect block. Run fsck ASAP!\n");
        return -EIO;
    }

    /* We're going to need this buffer... */
    if(!(iblock = (uint32_t *)malloc(fs->block_size)))
        /* Uh oh... */
        return -ENOMEM;

    /* Read the indirect block in. No need to cache this, since we're going to
       be releasing it very soon anyway... */
    if(ext2_block_read_nc(fs, iblk, (uint8_t *)iblock)) {
        free(iblock);
        return -EIO;
    }

    for(i = 0; i < blks_per_ind; ++i) {
        if(!(blk = iblock[i]))
            continue;

        if((rv = mark_block_free(fs, blk))) {
            free(iblock);
            return rv;
        }
    }

    /* Mark the indirect block itself free... */
    rv = mark_block_free(fs, iblk);
    free(iblock);
    return rv;
}

static int free_dind_block(ext2_fs_t *fs, ext2_inode_t *inode, uint32_t iblk) {
    uint32_t blks_per_ind = fs->block_size >> 2;
    uint32_t j;
    uint32_t *ib2;
    int rv;

    if(!iblk) {
        /* ????: This really shouldn't happen! */
        dbglog(DBG_ERROR, "ext2_inode_free_all: inode indicates use of block 0 "
               "for a doubly-indirect block. Run fsck ASAP!\n");
        return -EIO;
    }

    /* We're going to need this buffer... */
    if(!(ib2 = (uint32_t *)malloc(fs->block_size)))
        /* Uh oh... */
        return -ENOMEM;

    /* Read the doubly-indirect block in. No need to cache this, since we're
       going to be releasing it very soon anyway...  */
    if(ext2_block_read_nc(fs, iblk, (uint8_t *)ib2)) {
        free(ib2);
        return -EIO;
    }

    /* Go through each entry in the block and free all of its blocks. */
    for(j = 0; j < blks_per_ind; ++j) {
        if(ib2[j] && (rv = free_ind_block(fs, inode, ib2[j]))) {
            free(ib2);
            return rv;
        }
    }

    /* Mark the doubly-indirect block itself free... */
    rv = mark_block_free(fs, iblk);
    free(ib2);
    return rv;
}

static int free_tind_block(ext2_fs_t *fs, ext2_inode_t *inode, uint32_t iblk) {
    uint32_t blks_per_ind = fs->block_size >> 2;
    uint32_t j;
    uint32_t *ib2;
    int rv;

    if(!iblk) {
        /* ????: This really shouldn't happen! */
        dbglog(DBG_ERROR, "ext2_inode_free_all: inode indicates use of block 0 "
               "for the trebly-indirect block. Run fsck ASAP!\n");
        return -EIO;
    }

    /* We're going to need this buffer... */
    if(!(ib2 = (uint32_t *)malloc(fs->block_size)))
        /* Uh oh... */
        return -ENOMEM;

    /* Read the trebly-indirect block in. No need to cache this, since we're
       going to be releasing it very soon anyway... */
    if(ext2_block_read_nc(fs, iblk, (uint8_t *)ib2)) {
        free(ib2);
        return -EIO;
    }

    /* Go through each entry in the block and free all of its blocks. */
    for(j = 0; j < blks_per_ind; ++j) {
        if(ib2[j] && (rv = free_dind_block(fs, inode, ib2[j]))) {
            free(ib2);
            return rv;
        }
    }

    /* Mark the trebly-indirect block itself free... */
    rv = mark_block_free(fs, iblk);
    free(ib2);
    return rv;
}

int ext2_inode_free_all(ext2_fs_t *fs, ext2_inode_t *inode,
                        uint32_t inode_num, int for_del) {
    uint32_t bg, index, blk;
    uint8_t *buf;
    uint32_t i;
    int rv = 0, sub = (2 << fs->sb.s_log_block_size);
    struct int_inode *iinode = (struct int_inode *)inode;
    ext2_xattr_hdr_t *xattr;

    /* Do a write-back on the block cache... */
    if((rv = ext2_block_cache_wb(fs)))
        return rv;

    if(for_del) {
        /* Figure out what block group and index within that group the inode in
           question is. */
        bg = (inode_num - 1) / fs->sb.s_inodes_per_group;
        index = (inode_num - 1) % fs->sb.s_inodes_per_group;

        if(!(buf = ext2_block_read(fs, fs->bg[bg].bg_inode_bitmap, &rv)))
            return -EIO;

        /* Mark the inode as free in the bitmap and increase the counters. */
        ext2_bit_clear((uint32_t *)buf, index);
        ext2_block_mark_dirty(fs, fs->bg[bg].bg_inode_bitmap);

        ++fs->bg[bg].bg_free_inodes_count;
        ++fs->sb.s_free_inodes_count;
        fs->flags |= EXT2_FS_FLAG_SB_DIRTY;

        /* Set the deletion time of the inode */
        inode->i_dtime = (uint32_t)time(NULL);
        iinode->flags |= INODE_FLAG_DIRTY;
    }

    /* First look to see if there's any extended attributes... If so, free them
       up first.
       TODO: Should this only be checked for files, or can directories have
       extended attributes too? For now, assume that both can until we find some
       reason that assumption fails. */
    if(inode->i_file_acl && for_del) {
        if(!(buf = ext2_block_read(fs, inode->i_file_acl, &rv)))
            return -EIO;

        xattr = (ext2_xattr_hdr_t *)buf;

        if(xattr->h_magic != EXT2_XATTR_MAGIC) {
            dbglog(DBG_WARNING, "ext2_inode_free_all: xattr with bad magic!\n");
        }
        else if(!--xattr->h_refcount) {
            if((rv = mark_block_free(fs, inode->i_file_acl)))
                return rv;
        }

        ext2_block_mark_dirty(fs, inode->i_file_acl);
    }

    /* Free the direct data blocks. Note that since fast symlinks have the
       i_blocks field in their inodes set to 0, we don't have to do anything
       special to handle them in this code. */
    for(i = 0; i < 12; ++i) {
        if(!(blk = inode->i_block[i]))
            continue;

        if((rv = mark_block_free(fs, blk)))
            goto done;

        inode->i_block[i] = 0;
        fs->flags |= EXT2_FS_FLAG_SB_DIRTY;
    }

    /* Handle the singly-indirect block */
    if(inode->i_block[12] &&
       (rv = free_ind_block(fs, inode, inode->i_block[12])))
        goto done;

    inode->i_block[12] = 0;

    /* Time to go through the doubly-indirect block... */
    if(inode->i_block[13] &&
       (rv = free_dind_block(fs, inode, inode->i_block[13])))
        goto done;

    inode->i_block[13] = 0;

    /* Ugh... Really... A trebly-indirect block? At least we know we're done at
       this point... */
    if(inode->i_block[14])
        rv = free_tind_block(fs, inode, inode->i_block[14]);

    inode->i_block[14] = 0;

done:
    /* Restore the xattr block to the block count if needed. */
    if(inode->i_file_acl && !for_del)
        inode->i_blocks = sub;
    else
        inode->i_blocks = 0;

    return rv;
}

int ext2_inode_deref(ext2_fs_t *fs, uint32_t inode_num, int isdir) {
    ext2_inode_t *inode;
    int rv = 0;
    uint32_t bg;

    /* Don't even bother if we're mounted read-only. */
    if(!(fs->mnt_flags & EXT2FS_MNT_FLAG_RW))
        return -EROFS;

    /* Grab the inode first */
    if(!(inode = ext2_inode_get(fs, inode_num, &rv)))
        return rv;

    if(!isdir) {
        /* Decrement the reference count and force a write-back now */
        --inode->i_links_count;
    }
    else {
        /* ext2 doesn't allow hard links to directories, so if we're calling
           this on a directory, make sure the link count goes to 0 (directories
           actually hold a link to themselves through the '.' entry, so their
           link count should normally be 2 when calling this function). */
        inode->i_links_count = 0;

        /* We need to decrement the directories count on the block group
           descriptor as well. Might as well do it now. */
        bg = (inode_num - 1) / fs->sb.s_inodes_per_group;
        --fs->bg[bg].bg_used_dirs_count;
        fs->flags |= EXT2_FS_FLAG_SB_DIRTY;
    }

    if((rv = ext2_inode_wb((struct int_inode *)inode)))
        return rv;

    /* If the inode is not referenced anywhere anymore, free it */
    if(!inode->i_links_count)
        rv = ext2_inode_free_all(fs, inode, inode_num, 1);

    /* Release our reference to the inode, putting it in the free pool */
    ext2_inode_put(inode);

    return rv;
}

static uint8_t *alloc_direct_blk(ext2_fs_t *fs, struct int_inode *inode,
                                 uint32_t bg, uint32_t *rbn, int *err) {
    uint8_t *buf;
    uint32_t bn;

    if(!(buf = ext2_block_alloc(fs, bg, &bn, err)))
        return NULL;

    *rbn = bn;
    inode->inode.i_blocks += 2 << fs->sb.s_log_block_size;
    inode->flags |= INODE_FLAG_DIRTY;

    return buf;
}

static uint8_t *alloc_ind_blk(ext2_fs_t *fs, struct int_inode *inode,
                              uint32_t bg, uint32_t *rbn, int *err) {
    uint8_t *buf;
    uint32_t *buf32;
    uint32_t bn, bn2;

    /* Allocate the indirect block */
    if(!(buf = ext2_block_alloc(fs, bg, &bn, err)))
        return NULL;

    buf32 = (uint32_t *)buf;

    /* Allocate the direct block and update the inode */
    if(!(buf = alloc_direct_blk(fs, inode, bg, &bn2, err))) {
        mark_block_free(fs, bn);
        return NULL;
    }

    buf32[0] = bn2;
    *rbn = bn;
    inode->inode.i_blocks += 2 << fs->sb.s_log_block_size;
    inode->flags |= INODE_FLAG_DIRTY;

    return buf;
}

static uint8_t *alloc_dind_blk(ext2_fs_t *fs, struct int_inode *inode,
                               uint32_t bg, uint32_t *rbn, int *err) {
    uint8_t *buf;
    uint32_t *buf32;
    uint32_t bn, bn2;

    /* Allocate the double indirect block */
    if(!(buf = ext2_block_alloc(fs, bg, &bn, err)))
        return NULL;

    buf32 = (uint32_t *)buf;

    /* Allocate the indirect and direct blocks and update the inode */
    if(!(buf = alloc_ind_blk(fs, inode, bg, &bn2, err))) {
        mark_block_free(fs, bn);
        return NULL;
    }

    buf32[0] = bn2;
    *rbn = bn;
    inode->inode.i_blocks += 2 << fs->sb.s_log_block_size;
    inode->flags |= INODE_FLAG_DIRTY;

    return buf;
}

static uint8_t *alloc_tind_blk(ext2_fs_t *fs, struct int_inode *inode,
                               uint32_t bg, uint32_t *rbn, int *err) {
    uint8_t *buf;
    uint32_t *buf32;
    uint32_t bn, bn2;

    /* Allocate the double indirect block */
    if(!(buf = ext2_block_alloc(fs, bg, &bn, err)))
        return NULL;

    buf32 = (uint32_t *)buf;

    /* Allocate the double indirect, indirect, and direct blocks and update the
       inode */
    if(!(buf = alloc_dind_blk(fs, inode, bg, &bn2, err))) {
        mark_block_free(fs, bn);
        return NULL;
    }

    buf32[0] = bn2;
    *rbn = bn;
    inode->inode.i_blocks += 2 << fs->sb.s_log_block_size;
    inode->flags |= INODE_FLAG_DIRTY;

    return buf;
}

uint8_t *ext2_inode_alloc_block(ext2_fs_t *fs, ext2_inode_t *inode,
                                uint32_t blocks, int *err) {
    struct int_inode *iinode = (struct int_inode *)inode;
    uint8_t *buf;
    uint32_t *ind, *ind2, *ind3;
    uint32_t bg, ibn, ibn2, ibn3;
    uint32_t blocks_per_ind = fs->block_size >> 2;

    /* Don't even bother if we're mounted read-only. */
    if(!(fs->mnt_flags & EXT2FS_MNT_FLAG_RW)) {
        *err = EROFS;
        return NULL;
    }

    /* Subtract out the xattr block if there is one. */
    if(inode->i_file_acl)
        blocks -= 1;

    bg = (iinode->inode_num - 1) / fs->sb.s_inodes_per_group;

    /* First, see if we have a slot in the direct blocks open still. */
    if(blocks < 12) {
        return alloc_direct_blk(fs, iinode, bg, &inode->i_block[blocks], err);
    }
    else if(blocks == 12) {
        return alloc_ind_blk(fs, iinode, bg, &inode->i_block[12], err);
    }

    blocks -= 12;

    /* Do we have space in the current indirect block? */
    if(blocks < blocks_per_ind) {
        /* Read the indirect block in. */
        if(!(ind = (uint32_t *)ext2_block_read(fs, inode->i_block[12], err))) {
            return NULL;
        }

        /* Allocate the data block. */
        if((buf = alloc_direct_blk(fs, iinode, bg, &ind[blocks], err)))
            ext2_block_mark_dirty(fs, inode->i_block[12]);

        return buf;
    }
    else if(blocks == blocks_per_ind) {
        return alloc_dind_blk(fs, iinode, bg, &inode->i_block[13], err);
    }

    blocks -= blocks_per_ind;

    /* Do we have space in the first level of the doubly-indirect block? */
    if(blocks < (blocks_per_ind * blocks_per_ind)) {
        /* Read the double indirect block in. */
        if(!(ind2 = (uint32_t *)ext2_block_read(fs, inode->i_block[13], err)))
            return NULL;

        /* Figure out what entry we want in here... */
        ibn = blocks / blocks_per_ind;
        blocks %= blocks_per_ind;

        if(blocks) {
            if(!(ind = (uint32_t *)ext2_block_read(fs, ind2[ibn], err)))
                return NULL;

            /* Allocate the data block. */
            if((buf = alloc_direct_blk(fs, iinode, bg, &ind[blocks], err)))
                ext2_block_mark_dirty(fs, ind2[ibn]);

            return buf;
        }
        else {
            if((buf = alloc_ind_blk(fs, iinode, bg, &ind2[ibn], err)))
                ext2_block_mark_dirty(fs, inode->i_block[13]);

            return buf;
        }
    }
    else if(blocks == (blocks_per_ind * blocks_per_ind)) {
        return alloc_tind_blk(fs, iinode, bg, &inode->i_block[14], err);
    }

    /* So, it comes to this... */
    blocks -= blocks_per_ind * blocks_per_ind;
    ibn3 = blocks / (blocks_per_ind * blocks_per_ind);
    ibn2 = (blocks % (blocks_per_ind * blocks_per_ind)) / blocks_per_ind;
    ibn = blocks % blocks_per_ind;

    /* Do we have the EXTREMELY unlikely case that we've actually filled up the
       trebly indirect block?! */
    if(!ibn3 && !ibn2 && !ibn) {
        *err = EFBIG;
        return NULL;
    }
    /* Do we have to allocate a new doubly-indirect block? */
    else if(!ibn2 && !blocks) {
        if(!(ind3 = (uint32_t *)ext2_block_read(fs, inode->i_block[14], err)))
            return NULL;

        if((buf = alloc_dind_blk(fs, iinode, bg, &ind3[ibn3], err)))
            ext2_block_mark_dirty(fs, inode->i_block[14]);

        return buf;
    }
    /* What about a singly-indirect one? */
    else if(!ibn) {
        if(!(ind3 = (uint32_t *)ext2_block_read(fs, inode->i_block[14], err)))
            return NULL;

        if(!(ind2 = (uint32_t *)ext2_block_read(fs, ind3[ibn3], err)))
            return NULL;

        if((buf = alloc_ind_blk(fs, iinode, bg, &ind2[ibn2], err)))
            ext2_block_mark_dirty(fs, ind3[ibn3]);

        return buf;
    }
    /* We just need the data block if we get here. */
    else {
        if(!(ind3 = (uint32_t *)ext2_block_read(fs, inode->i_block[14], err)))
            return NULL;

        if(!(ind2 = (uint32_t *)ext2_block_read(fs, ind3[ibn3], err)))
            return NULL;

        if(!(ind = (uint32_t *)ext2_block_read(fs, ind2[ibn2], err)))
            return NULL;

        if((buf = alloc_direct_blk(fs, iinode, bg, &ind[ibn], err)))
            ext2_block_mark_dirty(fs, ind2[ibn2]);

        return buf;
    }
}

static ext2_dirent_t *search_dir(uint8_t *buf, int block_size,
                                 const char *token, int *err) {
    int block_offset = 0;
    ext2_dirent_t *dent;
    char name[256];

    while(block_offset < block_size) {
        dent = (ext2_dirent_t *)(buf + block_offset);

        /* Make sure we don't trip and fall on a malformed entry. */
        if(!dent->rec_len) {
            *err = -EIO;
            return NULL;
        }

        if(dent->inode) {
            /* See if this is what we're looking for. */
            memcpy(name, dent->name, dent->name_len);
            name[dent->name_len] = 0;

            if(!strcmp(name, token))
                return dent;
        }

        block_offset += dent->rec_len;
    }

    return NULL;
}

static ext2_dirent_t *search_indir(ext2_fs_t *fs, const uint32_t *iblock,
                                   int block_size, const char *token,
                                   int *err) {
    uint8_t *buf;
    int i, block_ents;
    ext2_dirent_t *rv;

    /* We're going to need this buffer... */
    if(!(buf = (uint8_t *)malloc(block_size))) {
        *err = -ENOMEM;
        return NULL;
    }

    block_ents = block_size >> 2;

    /* Search through each block until we get to the end. */
    for(i = 0; i < block_ents && iblock[i]; ++i) {
        if(ext2_block_read_nc(fs, iblock[i], buf)) {
            free(buf);
            *err = -EIO;
            return NULL;
        }

        if((rv = search_dir(buf, block_size, token, err))) {
            free(buf);
            *err = 0;
            return rv;
        }
        else if(*err) {
            free(buf);
            return NULL;
        }
    }

    free(buf);
    *err = 0;
    return NULL;
}

static ext2_dirent_t *search_indir_23(ext2_fs_t *fs, const uint32_t *iblock,
                                      int block_size, const char *token,
                                      int *err, int triple) {
    uint32_t *buf;
    int i, block_ents;
    ext2_dirent_t *rv;

    /* We're going to need this buffer... */
    if(!(buf = (uint32_t *)malloc(block_size))) {
        *err = -ENOMEM;
        return NULL;
    }

    block_ents = block_size >> 2;

    if(!triple) {
        /* Search through each indirect block until we get to the end. */
        for(i = 0; i < block_ents && iblock[i]; ++i) {
            if(ext2_block_read_nc(fs, iblock[i], (uint8_t *)buf)) {
                free(buf);
                *err = -EIO;
                return NULL;
            }

            if((rv = search_indir(fs, buf, block_size, token, err))) {
                free(buf);
                *err = 0;
                return rv;
            }
        }
    }
    else {
        /* Search through each doubly-indirect block until we get to the end. */
        for(i = 0; i < block_size && iblock[i]; ++i) {
            if(ext2_block_read_nc(fs, iblock[i], (uint8_t *)buf)) {
                free(buf);
                *err = -EIO;
                return NULL;
            }

            if((rv = search_indir_23(fs, buf, block_size, token, err, 0))) {
                free(buf);
                *err = 0;
                return rv;
            }
        }
    }

    free(buf);
    *err = 0;
    return NULL;
}

int ext2_inode_by_path(ext2_fs_t *fs, const char *path, ext2_inode_t **rv,
                       uint32_t *inode_num, int rlink, ext2_dirent_t **rdent) {
    ext2_inode_t *inode, *last;
    char *ipath, *cxt, *token;
    int blocks, i, block_size;
    uint8_t *buf;
    uint32_t *ib;
    ext2_dirent_t *dent = NULL;
    int err = 0;
    size_t tmp_sz;
    char *symbuf;
    int links_derefed = 0;

    if(!path || !fs || !rv)
        return -EFAULT;

    /* Read the root directory inode first. */
    if(!(inode = ext2_inode_get(fs, EXT2_ROOT_INO, &err)))
        return err;

    /* We're going to tokenize the string into its component parts, so make a
       copy of the path to go from here. */
    if(!(ipath = strdup(path))) {
        ext2_inode_put(inode);
        return -ENOMEM;
    }

    token = strtok_r(ipath, "/", &cxt);

    /* If we get nothing back here, they gave us /. Give them back the root
       directory inode. */
    if(!token) {
        free(ipath);
        *rv = inode;
        *inode_num = EXT2_ROOT_INO;

        if(rdent)
            *rdent = NULL;
        return 0;
    }

    block_size = fs->block_size;

    while(token) {
        last = inode;

        /* If this isn't a directory, give up now. */
        if(!(inode->i_mode & EXT2_S_IFDIR)) {
            free(ipath);
            ext2_inode_put(inode);
            return -ENOTDIR;
        }

        blocks = inode->i_blocks / (2 << fs->sb.s_log_block_size);

        /* Run through any direct blocks in the inode. */
        for(i = 0; i < blocks && inode->i_block[i] && i < 12; ++i) {
            /* Grab the block, looking in the directory cache. */
            if(!(buf = ext2_block_read(fs, inode->i_block[i], &err))) {
                free(ipath);
                ext2_inode_put(inode);
                return -EIO;
            }

            /* Search through the directory block */
            if((dent = search_dir(buf, block_size, token, &err))) {
                goto next_token;
            }
            else if(err) {
                free(ipath);
                ext2_inode_put(inode);
                return err;
            }
        }

        /* Short circuit... */
        if(!inode->i_block[i])
            goto out;

        /* Next, look through the indirect block. */
        if(!(ib = (uint32_t *)ext2_block_read(fs, inode->i_block[12], &err))) {
            free(ipath);
            ext2_inode_put(inode);
            return -EIO;
        }

        if((dent = search_indir(fs, ib, block_size, token, &err))) {
            goto next_token;
        }
        else if(err) {
            free(ipath);
            ext2_inode_put(inode);
            return err;
        }

        /* Next, look through the doubly-indirect block. */
        if(inode->i_block[13]) {
            /* Grab the block, looking in the directory cache. */
            if(!(ib = (uint32_t *)ext2_block_read(fs, inode->i_block[13],
                                                  &err))) {
                free(ipath);
                ext2_inode_put(inode);
                return -EIO;
            }

            if((dent = search_indir_23(fs, ib, block_size, token, &err, 0))) {
                goto next_token;
            }
            else if(err) {
                free(ipath);
                ext2_inode_put(inode);
                return err;
            }
        }

        /* Finally, try the triply-indirect block... God help us if we actually
           have to look all the way through one of these... */
        if(inode->i_block[14]) {
            /* Grab the block, looking in the directory cache. */
            if(!(ib = (uint32_t *)ext2_block_read(fs, inode->i_block[14],
                                                  &err))) {
                free(ipath);
                ext2_inode_put(inode);
                return -EIO;
            }

            if((dent = search_indir_23(fs, ib, block_size, token, &err, 1))) {
                goto next_token;
            }
            else if(err) {
                free(ipath);
                ext2_inode_put(inode);
                return err;
            }
        }

out:
        /* If we get here, we didn't find the next entry. Return that error. */
        ext2_inode_put(inode);

        if((token = strtok_r(NULL, "/", &cxt))) {
            free(ipath);
            return -ENOTDIR;
        }
        else {
            free(ipath);
            return -ENOENT;
        }

next_token:
        token = strtok_r(NULL, "/", &cxt);

        if(!(inode = ext2_inode_get(fs, dent->inode, &err))) {
            free(ipath);
            ext2_inode_put(last);
            return err;
        }

        /* Are we supposed to resolve symbolic links? If we have one and we're
           supposed to resolve them, do it. */
        if((inode->i_mode & 0xF000) == EXT2_S_IFLNK &&
           (rlink == 1 || (rlink == 2 && token))) {
            /* Make sure we don't fall into an infinite loop... */
            if(links_derefed++ > SYMLOOP_MAX) {
                free(ipath);
                ext2_inode_put(inode);
                return -ELOOP;
            }

            tmp_sz = PATH_MAX;

            if(!(symbuf = (char *)malloc(PATH_MAX))) {
                free(ipath);
                ext2_inode_put(inode);
                return -ENOMEM;
            }

            if(ext2_resolve_symlink(fs, inode, symbuf, &tmp_sz)) {
                free(symbuf);
                free(ipath);
                ext2_inode_put(inode);
                return -EIO;
            }

            /* Make sure we got it all */
            if(tmp_sz >= PATH_MAX) {
                free(symbuf);
                free(ipath);
                ext2_inode_put(inode);
                return -ENAMETOOLONG;
            }

            /* For now, drop any absolute pathnames that we might encounter. At
               some point, I'll probably revisit this decision, but for now
               that's just how it is. */
            if(symbuf[0] == '/') {
                free(symbuf);
                free(ipath);
                ext2_inode_put(inode);
                return -EXDEV;
            }

            /* Tack on the rest of the path to the symbolic link. This is
               horribly inefficient, but it should work fine. */
            while(token) {
                if((tmp_sz += strlen(token) + 1) >= PATH_MAX) {
                    free(symbuf);
                    free(ipath);
                    ext2_inode_put(inode);
                    return -ENAMETOOLONG;
                }

                strcat(symbuf, "/");
                strcat(symbuf, token);
                token = strtok_r(NULL, "/", &cxt);
            }

            /* Continue our search for the object in question, now that we've
               resolved the link... */
            free(ipath);
            ipath = symbuf;
            token = strtok_r(ipath, "/", &cxt);
            ext2_inode_put(inode);
            inode = last;
        }
        else {
            ext2_inode_put(last);
        }
    }

    /* Well, looks like we have it, return the inode. */
    *rv = inode;
    *inode_num = dent->inode;
    free(ipath);

    if(rdent)
        *rdent = dent;
    return 0;
}

uint8_t *ext2_inode_read_block(ext2_fs_t *fs, const ext2_inode_t *inode,
                               uint32_t block_num, uint32_t *r_block,
                               int *err) {
    uint32_t blks_per_ind, ibn;
    uint32_t *iblock;
    int shift = 1 + fs->sb.s_log_block_size;
    uint64_t sz;

    /* Grab the size */
    if((inode->i_mode & 0xF000) == EXT2_S_IFREG)
        sz = ext2_inode_size(inode);
    else
        sz = (uint64_t)inode->i_size;

    /* Check to be sure we're not being asked to do something stupid... */
    if((block_num << (shift + 9)) >= sz) {
        *err = EINVAL;
        return NULL;
    }

    /* If we're reading a direct block, this is easy. */
    if(block_num < 12) {
        if(r_block)
            *r_block = inode->i_block[block_num];

        return ext2_block_read(fs, inode->i_block[block_num], err);
    }

    blks_per_ind = fs->block_size >> 2;
    block_num -= 12;

    /* Are we looking at the singly-indirect block? */
    if(block_num < blks_per_ind) {
        if(!(iblock = (uint32_t *)ext2_block_read(fs, inode->i_block[12], err)))
            return NULL;

        if(r_block)
            *r_block = iblock[block_num];

        return ext2_block_read(fs, iblock[block_num], err);
    }

    /* Ok, we're looking at at least a doubly-indirect block... */
    block_num -= blks_per_ind;
    if(block_num < (blks_per_ind * blks_per_ind)) {
        if(!(iblock = (uint32_t *)ext2_block_read(fs, inode->i_block[13], err)))
            return NULL;

        /* Figure out what entry we want in here... */
        ibn = block_num / blks_per_ind;
        block_num %= blks_per_ind;

        if(!(iblock = (uint32_t *)ext2_block_read(fs, iblock[ibn], err)))
            return NULL;

        /* Ok... Now we should be good to go. */
        if(r_block)
            *r_block = iblock[block_num];

        return ext2_block_read(fs, iblock[block_num], err);
    }

    /* Ugh... You're going to make me look at a triply-indirect block now? */
    block_num -= blks_per_ind * blks_per_ind;
    if(!(iblock = (uint32_t *)ext2_block_read(fs, inode->i_block[14], err)))
        return NULL;

    /* Figure out what entry we want in here... */
    ibn = block_num / blks_per_ind;
    block_num %= blks_per_ind;

    if(!(iblock = (uint32_t *)ext2_block_read(fs, iblock[ibn], err)))
        return NULL;

    /* And in this one too... */
    ibn = block_num / blks_per_ind;
    block_num %= blks_per_ind;

    if(!(iblock = (uint32_t *)ext2_block_read(fs, iblock[ibn], err)))
        return NULL;

    /* Ok... Now we should be good to go. Finally. */
    if(block_num < blks_per_ind) {
        if(r_block)
            *r_block = iblock[block_num];

        return ext2_block_read(fs, iblock[block_num], err);
    }
    else {
        /* This really shouldn't happen... */
        *err = EIO;
        return NULL;
    }
}
