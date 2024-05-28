/* KallistiOS ##version##

   inode.h
   Copyright (C) 2012, 2013 Lawrence Sebald
*/

#ifndef __EXT2_INODE_H
#define __EXT2_INODE_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

#include "ext2fs.h"
#include "directory.h"

typedef struct ext2_inode {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15];
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    struct {
        uint8_t l_i_frag;
        uint8_t l_i_fsize;
        uint16_t reserved;
        uint16_t l_i_uid_high;
        uint16_t l_i_gid_high;
        uint32_t reserved2;
    } i_osd2;
} ext2_inode_t;

typedef struct ext2_xattr_hdr {
    uint32_t h_magic;
    uint32_t h_refcount;
    uint32_t h_blocks;
    uint32_t h_hash;
    uint8_t reserved[16];
} ext2_xattr_hdr_t;

/* i_mode values */
#define EXT2_S_IFSOCK   0xC000
#define EXT2_S_IFLNK    0xA000
#define EXT2_S_IFREG    0x8000
#define EXT2_S_IFBLK    0x6000
#define EXT2_S_IFDIR    0x4000
#define EXT2_S_IFCHR    0x2000
#define EXT2_S_IFIFO    0x1000

#define EXT2_S_ISUID    0x0800
#define EXT2_S_ISGID    0x0400
#define EXT2_S_ISVTX    0x0200

#define EXT2_S_IRUSR    0x0100
#define EXT2_S_IWUSR    0x0080
#define EXT2_S_IXUSR    0x0040
#define EXT2_S_IRGRP    0x0020
#define EXT2_S_IWGRP    0x0010
#define EXT2_S_IXGRP    0x0008
#define EXT2_S_IROTH    0x0004
#define EXT2_S_IWOTH    0x0002
#define EXT2_S_IXOTH    0x0001

/* i_flags values */
#define EXT2_SECRM_FL           0x00000001
#define EXT2_UNRM_FL            0x00000002
#define EXT2_COMPR_FL           0x00000004
#define EXT2_SYNC_FL            0x00000008
#define EXT2_IMMUTABLE_FL       0x00000010
#define EXT2_APPEND_FL          0x00000020
#define EXT2_NODUMP_FL          0x00000040
#define EXT2_NOATIME_FL         0x00000080
#define EXT2_DIRTY_FL           0x00000100
#define EXT2_COMPRBLK_FL        0x00000200
#define EXT2_NOCOMPR_FL         0x00000400
#define EXT2_ECOMPR_FL          0x00000800
#define EXT2_BTREE_FL           0x00001000
#define EXT2_INDEX_FL           EXT2_BTREE_FL
#define EXT2_IMAGIC_FL          0x00002000
#define EXT2_JOURNAL_DATA_FL    0x00004000
#define EXT2_RESERVED_FL        0x80000000

/* Reserved inodes */
#define EXT2_BAD_INO            1
#define EXT2_ROOT_INO           2
#define EXT2_ACL_IDX_INO        3
#define EXT2_ACL_DATA_INO       4
#define EXT2_BOOT_LOADER_INO    5
#define EXT2_UNDEL_DIR_INO      6

/* This should be in the h_magic of an xattr */
#define EXT2_XATTR_MAGIC        0xEA020000

/* Get the size of a regular file inode as a 64-bit integer. */
static inline uint64_t ext2_inode_size(const ext2_inode_t *inode) {
    return (uint64_t)(inode->i_size) | ((uint64_t)(inode->i_dir_acl) << 32);
}

/* Set the size of a regular file from a 64-bit integer. */
static inline void ext2_inode_set_size(ext2_inode_t *inode, uint64_t sz) {
    inode->i_size = (uint32_t)sz;
    inode->i_dir_acl = (uint32_t)(sz >> 32);
}

/* Initialize the inode cache. This will be called by ext2_init(). */
void ext2_inode_init(void);

/* Get a reference to an inode. When calling either of these, you must release
   the inode you get back with ext2_inode_put. */
ext2_inode_t *ext2_inode_get(ext2_fs_t *fs, uint32_t inode_num, int *err);
int ext2_inode_by_path(ext2_fs_t *fs, const char *path, ext2_inode_t **rv,
                       uint32_t *inode_num, int rlink, ext2_dirent_t **rdent);

void ext2_inode_put(ext2_inode_t *inode);

/* Increment the reference count on an inode that you already have a reference
   to. */
void ext2_inode_retain(ext2_inode_t *inode);

void ext2_inode_mark_dirty(ext2_inode_t *inode);

/* Write-back all of the inodes marked as dirty from the specified filesystem to
   its block cache. */
int ext2_inode_cache_wb(ext2_fs_t *fs);

/* Allocate an unused inode on the specified filesystem. */
ext2_inode_t *ext2_inode_alloc(ext2_fs_t *fs, uint32_t parent, int *err,
                               uint32_t *ninode);

/* Free all data blocks associated with an inode. */
int ext2_inode_free_all(ext2_fs_t *fs, ext2_inode_t *inode,
                        uint32_t inode_num, int fxattr);

/* Decrease the reference count on an inode. If the reference count reaches
   zero, deallocate the inode and all of its blocks. */
int ext2_inode_deref(ext2_fs_t *fs, uint32_t inode_num, int isdir);

/* Allocate a new data block for an inode, filling in the blocks array and
   updating the block count. It is the caller's responsibility to update the
   i_size and any timestamps needed. */
uint8_t *ext2_inode_alloc_block(ext2_fs_t *fs, ext2_inode_t *inode,
                                uint32_t blocks,int *err);

uint8_t *ext2_inode_read_block(ext2_fs_t *fs, const ext2_inode_t *inode,
                               uint32_t block_num, uint32_t *r_block,
                               int *err);

/* In symlink.c */
int ext2_resolve_symlink(ext2_fs_t *fs, ext2_inode_t *inode, char *rv,
                         size_t *rv_len);

__END_DECLS

#endif /* !__EXT2_INODE_H */
