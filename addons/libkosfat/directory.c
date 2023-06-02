/* KallistiOS ##version##

   directory.c
   Copyright (C) 2019 Lawrence Sebald
*/

#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <time.h>

#include "fatfs.h"
#include "ucs.h"
#include "directory.h"
#include "fatinternal.h"

#if __GNUC__ >= 9
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#endif

#ifdef __STRICT_ANSI__
/* These don't necessarily get prototyped in string.h in standard-compliant mode
   as they are extensions to the standard. Declaring them this way shouldn't
   hurt. */
char *strtok_r(char *, const char *, char **);
char *strdup(const char *);
#endif

static uint16_t longname_buf[256], longname_buf2[256];

#define DOT_NAME    ".          "
#define DOTDOT_NAME "..         "

static uint8_t fat_shortname_checksum(char fn[11]) {
    uint8_t rv = fn[0];

    /* Rotate the existing value right by 1 and add the next character... */
    rv = ((rv << 7) | (rv >> 1))  + fn[1];
    rv = ((rv << 7) | (rv >> 1))  + fn[2];
    rv = ((rv << 7) | (rv >> 1))  + fn[3];
    rv = ((rv << 7) | (rv >> 1))  + fn[4];
    rv = ((rv << 7) | (rv >> 1))  + fn[5];
    rv = ((rv << 7) | (rv >> 1))  + fn[6];
    rv = ((rv << 7) | (rv >> 1))  + fn[7];
    rv = ((rv << 7) | (rv >> 1))  + fn[8];
    rv = ((rv << 7) | (rv >> 1))  + fn[9];
    rv = ((rv << 7) | (rv >> 1))  + fn[10];

    return rv;
}

static int fat_search_dir(fat_fs_t *fs, const char *fn, uint32_t cluster,
                          fat_dentry_t *rv, uint32_t *rcl, uint32_t *roff) {
    uint8_t *cl;
    int err, done = 0;
    uint32_t i, j = 0, max;
    fat_dentry_t *ent;

    /* Figure out how many directory entries there are in each cluster/block. */
    if(fs->sb.fs_type == FAT_FS_FAT32 || !(cluster & 0x80000000)) {
        /* Either we're working with a regular directory or we're working with
           the root directory on FAT32 (which is the same as a normal
           directory). We care about the number of entries per cluster. */
        max = (fs->sb.bytes_per_sector * fs->sb.sectors_per_cluster) >> 5;
    }
    else {
        /* We're working with the root directory on FAT12/FAT16, so we need to
           look at the number of entries per sector. */
        max = fs->sb.bytes_per_sector >> 5;
    }

    while(!done) {
        if(!(cl = fat_cluster_read(fs, cluster, &err))) {
            dbglog(DBG_ERROR, "Error reading directory at cluster %" PRIu32
                   ": %s\n", cluster, strerror(err));
            return -EIO;
        }

        for(i = 0; i < max && !done; ++i, ++j) {
            ent = (fat_dentry_t *)(cl + (i << 5));

            /* If name[0] is zero, then we've hit the end of the directory. */
            if(ent->name[0] == FAT_ENTRY_EOD) {
                return -ENOENT;
            }
            /* If name[0] == 0xE5, then this entry is empty (but there might
               still be additional entries after it). */
            else if(ent->name[0] == FAT_ENTRY_FREE) {
                continue;
            }
            /* Ignore long name entries. */
            else if(FAT_IS_LONG_NAME(ent)) {
                continue;
            }

            /* We've got something useful, let's see if it's what we're looking
               for... */
            if(!memcmp(fn, ent->name, 11)) {
                *rv = *ent;
                *rcl = cluster;
                *roff = (i << 5);
                return 0;
            }
        }

        if(!(cluster & 0x80000000)) {
            cluster = fat_read_fat(fs, cluster, &err);
            if(cluster == 0xFFFFFFFF)
                return -err;

            if(fat_is_eof(fs, cluster))
                done = 1;
        }
        else {
            ++cluster;

            if(j >= fs->sb.root_dir)
                done = 1;
        }
    }

    return -ENOENT;
}

static int read_longname(fat_fs_t *fs, uint32_t *cluster, uint32_t *offset,
                         uint32_t max, int32_t *max2) {
    int err, done = 0;
    uint32_t i, fnlen;
    fat_dentry_t *ent;
    fat_longname_t *lent;
    uint8_t *cl;

    ++*offset;

    while(!done) {
        if(!(cl = fat_cluster_read(fs, *cluster, &err))) {
            dbglog(DBG_ERROR, "Error reading directory at cluster %" PRIu32
                   ": %s\n", *cluster, strerror(err));
            return -EIO;
        }

        for(i = *offset; i < max; ++i) {
            ent = (fat_dentry_t *)(cl + (i << 5));

            /* If name[0] is zero, then we've hit the end of the directory. */
            if(ent->name[0] == FAT_ENTRY_EOD) {
                return -EIO;
            }
            /* If name[0] == 0xE5, then this entry is empty (but there might
               still be additional entries after it). */
            else if(ent->name[0] == FAT_ENTRY_FREE) {
                return -EIO;
            }
            /* Non-long name entries in the middle of a long name entry are
               bad... */
            else if(!FAT_IS_LONG_NAME(ent)) {
                return -EIO;
            }

            lent = (fat_longname_t *)ent;

            /* We've got our expected long name block... Deal with it. */
            fnlen = ((lent->order - 1) & 0x3F) * 13;

            /* Build out the filename component we have. */
            memcpy(&longname_buf[fnlen], lent->name1, 10);
            memcpy(&longname_buf[fnlen + 5], lent->name2, 12);
            memcpy(&longname_buf[fnlen + 11], lent->name3, 4);

            /* XXXX: Calculate the checksum here. */

            if((lent->order & 0x3F) == 1) {
                *offset = i;
                return 0;
            }
        }

        if(!(*cluster & 0x80000000)) {
            *cluster = fat_read_fat(fs, *cluster, &err);
            if(*cluster == 0xFFFFFFFF) {
                return -EIO;
            }

            if(fat_is_eof(fs, *cluster))
                done = 1;

            *offset = 0;
        }
        else {
            ++*cluster;

            *max2 -= max;
            if(*max2 <= 0) {
                return -EIO;
            }

            *offset = 0;
        }
    }

    return -EIO;
}

static int fat_search_long(fat_fs_t *fs, const char *fn, uint32_t cluster,
                           fat_dentry_t *rv, uint32_t *rcl, uint32_t *roff,
                           uint32_t *rlcl, uint32_t *rloff) {
    uint8_t *cl;
    int err, done = 0;
    uint32_t i = 0, max, skip = 0;
    int32_t max2;
    fat_dentry_t *ent;
    fat_longname_t *lent;
    size_t l = strlen(fn);
    uint32_t fnlen, lcl = 0, loff = 0, cluster2;

    /* Figure out how many directory entries there are in each cluster/block. */
    if(fs->sb.fs_type == FAT_FS_FAT32 || !(cluster & 0x80000000)) {
        /* Either we're working with a regular directory or we're working with
           the root directory on FAT32 (which is the same as a normal
           directory). We care about the number of entries per cluster. */
        max = (fs->sb.bytes_per_sector * fs->sb.sectors_per_cluster) >> 5;
        max2 = 0xFFFFFFFF;
    }
    else {
        /* We're working with the root directory on FAT12/FAT16, so we need to
           look at the number of entries per sector. */
        max = fs->sb.bytes_per_sector >> 5;
        max2 = (int32_t)fs->sb.root_dir;
    }

    fat_utf8_to_ucs2(longname_buf2, (const uint8_t *)fn, 256, l);

    while(!done) {
        if(!(cl = fat_cluster_read(fs, cluster, &err))) {
            dbglog(DBG_ERROR, "Error reading directory at cluster %" PRIu32
                   ": %s\n", cluster, strerror(err));
            return -EIO;
        }

        for(; i < max && !done; ++i) {
            cluster2 = cluster;
            ent = (fat_dentry_t *)(cl + (i << 5));

            /* Are we skipping this entry? */
            if(skip) {
                --skip;
                continue;
            }

            /* If name[0] is zero, then we've hit the end of the directory. */
            if(ent->name[0] == FAT_ENTRY_EOD) {
                return -ENOENT;
            }
            /* If name[0] == 0xE5, then this entry is empty (but there might
               still be additional entries after it). */
            else if(ent->name[0] == FAT_ENTRY_FREE) {
                continue;
            }
            /* Ignore entries that aren't long name entries... */
            else if(!FAT_IS_LONG_NAME(ent)) {
                continue;
            }

            lent = (fat_longname_t *)ent;

            /* We've got a long name entry, see if we even care about it (i.e,
               if the length is feasible and if the entry is sane). */
            if(!(lent->order & FAT_ORDER_LAST))
                continue;

            fnlen = (lent->order & 0x3F) * 13;
            if(l > fnlen) {
                skip = (lent->order & 0x3F);
                continue;
            }

            /* Build out the filename component we have. */
            fnlen -= 13;
            memcpy(&longname_buf[fnlen], lent->name1, 10);
            memcpy(&longname_buf[fnlen + 5], lent->name2, 12);
            memcpy(&longname_buf[fnlen + 11], lent->name3, 4);
            longname_buf[fnlen + 14] = 0;

            /* XXXX: Calculate the checksum here. */

            /* Now, is the filename length *actually* right? */
            fnlen += fat_strlen_ucs2(longname_buf + fnlen);
            if(l != fnlen) {
                skip = (lent->order & 0x3F);
                continue;
            }

            lcl = cluster;
            loff = i << 5;

            /* Is this the only long name entry needed? */
            if(lent->order != 0x41) {
                if(read_longname(fs, &cluster, &i, max, &max2))
                    return -EIO;
            }

            fat_ucs2_tolower(longname_buf, fnlen);
            fat_ucs2_tolower(longname_buf2, fnlen);

            if(!memcmp(longname_buf, longname_buf2, fnlen * sizeof(uint16_t))) {
                /* The next entry should be the dentry we want (that is to say,
                   the short name entry for this long name). */
                if(i < max) {
                    if(cluster != cluster2) {
                        if(!(cl = fat_cluster_read(fs, cluster, &err))) {
                            dbglog(DBG_ERROR, "Error reading directory at "
                                   "cluster %" PRIu32 ": %s\n", cluster,
                                   strerror(err));
                            return -EIO;
                        }
                    }

                    ent = (fat_dentry_t *)(cl + ((i + 1) << 5));

                    /* Make sure we got a valid short entry... */
                    if(ent->name[0] == FAT_ENTRY_EOD ||
                       ent->name[0] == FAT_ENTRY_FREE) {
                        return -ENOENT;
                    }

                    *rv = *ent;
                    *rcl = cluster;
                    *roff = ((i + 1) << 5);
                    *rlcl = lcl;
                    *rloff = loff;

                    return 0;
                }
                else {
                    /* Ugh. The short entry is in another cluster, so read it
                       in to return it. */
                    if(!(cluster & 0x80000000)) {
                        cluster = fat_read_fat(fs, cluster, &err);
                        if(cluster == 0xFFFFFFFF)
                            return -err;

                        if(fat_is_eof(fs, cluster))
                            return -EIO;
                    }
                    else {
                        ++cluster;
                        max2 -= max;

                        if(max2 <= 0)
                            return -EIO;
                    }

                    if(!(cl = fat_cluster_read(fs, cluster, &err))) {
                        dbglog(DBG_ERROR, "Error reading directory at cluster %"
                               PRIu32 ": %s\n", cluster, strerror(err));
                        return -EIO;
                    }

                    ent = (fat_dentry_t *)cl;

                    /* Make sure we got a valid short entry... */
                    if(ent->name[0] == FAT_ENTRY_EOD ||
                       ent->name[0] == FAT_ENTRY_FREE) {
                        return -ENOENT;
                    }

                    *rv = *ent;
                    *rcl = cluster;
                    *roff = 0;
                    *rlcl = lcl;
                    *rloff = loff;
                    return 0;
                }
            }
            else {
                skip = 1;

                if(cluster2 != cluster)
                    break;
            }
        }

        if(!(cluster & 0x80000000)) {
            cluster = fat_read_fat(fs, cluster, &err);
            if(cluster == 0xFFFFFFFF)
                return -err;

            if(fat_is_eof(fs, cluster))
                done = 1;
        }
        else {
            ++cluster;
            max2 -= max;

            if(max2 <= 0)
                done = 1;
        }
    }

    return -ENOENT;
}

static void normalize_shortname(const char *fn, char out[11]) {
    size_t l = strlen(fn), i;
    char *dot = strrchr(fn, '.');
    size_t le = 0;

    if(dot)
        le = strlen(dot) - 1;

    for(i = 0; i < 8 && i < l && dot != fn + i; ++i) {
        out[i] = toupper((int)fn[i]);
    }

    for(; i < 8; ++i) {
        out[i] = ' ';
    }

    for(i = 0; i < 3 && i < le; ++i) {
        out[8 + i] = toupper((int)dot[i + 1]);
    }

    for(; i < 3; ++i) {
        out[8 + i] = ' ';
    }
}

static int is_component_short(const char *fn) {
    size_t l = strlen(fn), i;
    char *dot = strrchr(fn, '.');
    int dotct = 0;

    /* 8.3 == 12 characters, so anything more than that can't be a shortname. */
    if(l > 12)
        return 0;

    /* Short filenames can't start with a dot. */
    if(dot == fn)
        return 0;

    /* If the basename of the component is more than 8 characters, then it
       can't be a 8.3 filename. */
    if(dot > fn + 8)
        return 0;

    if(dot) {
        /* If we have more than 4 characters including the dot, then it can't be
           a shortname. */
        if(strlen(dot) > 4)
            return 0;
    }
    else {
        /* We can't have more than 8 characters if there's no dot. */
        if(l > 8)
            return 0;
    }

    /* Short names can't have certain characters... */
    for(i = 0; i < l; ++i) {
        if(fn[i] == '+' || fn[i] == ',' || fn[i] == ';' || fn[i] == '[' ||
           fn[i] == ']' || fn[i] == ' ' || fn[i] == '=')
            return 0;

        if(fn[i] == '.')
            ++dotct;

        /* Technically, these characters aren't allowed in long names either. */
        if(fn[i] == '*' || fn[i] == ':' || fn[i] == '/' || fn[i] == '\\' ||
           fn[i] == '|' || fn[i] == '"' || fn[i] == '?' || fn[i] == '<' ||
           fn[i] == '>')
            return 0;
    }

    /* We can't have more than one dot in a shortname */
    if(dotct > 1)
        return 0;

    /* If we get here, it should be fine... */
    return 1;
}

static int fat_find_child2(fat_fs_t *fs, const char fn[11],
                           fat_dentry_t *parent) {
    uint32_t cl;
    uint32_t rcl, roff;
    fat_dentry_t tmp;

    cl = parent->cluster_low | (parent->cluster_high << 16);
    return fat_search_dir(fs, fn, cl, &tmp, &rcl, &roff);
}

int fat_find_child(fat_fs_t *fs, const char *fn, fat_dentry_t *parent,
                   fat_dentry_t *rv, uint32_t *rcl, uint32_t *roff,
                   uint32_t *rlcl, uint32_t *rloff) {
    char *fnc = strdup(fn), comp[11];
    uint32_t cl;
    int err;

    cl = parent->cluster_low | (parent->cluster_high << 16);

    if(is_component_short(fnc)) {
        normalize_shortname(fnc, comp);

        err = fat_search_dir(fs, comp, cl, rv, rcl, roff);
        *rlcl = 0;
        *rloff = 0;
    }
    else {
        err = fat_search_long(fs, fnc, cl, rv, rcl, roff, rlcl, rloff);
    }

    free(fnc);
    return err;
}

int fat_find_dentry(fat_fs_t *fs, const char *fn, fat_dentry_t *rv,
                    uint32_t *rcl, uint32_t *roff, uint32_t *rlcl,
                    uint32_t *rloff) {
    char *fnc = strdup(fn), comp[11], *tmp, *tok;
    int err = -ENOENT;
    fat_dentry_t cur;
    uint32_t cl, off, lcl = 0, loff = 0;

    /* First thing is first, tokenize the filename into components... */
    tok = strtok_r(fnc, "/", &tmp);

    /* If we don't get anything back here, they've asked for the root directory.
       Make up a dentry for it. */
    if(!tok) {
        memset(rv, 0, sizeof(fat_dentry_t));
        rv->attr = FAT_ATTR_DIRECTORY;

        if(fs->sb.fs_type == FAT_FS_FAT32) {
            rv->cluster_high = fs->sb.root_dir >> 16;
            rv->cluster_low = (uint16_t)fs->sb.root_dir;
        }
        else {
            rv->cluster_high = 0x8000;
            rv->cluster_low = (uint16_t)(fs->sb.reserved_sectors +
                                         (fs->sb.num_fats * fs->sb.fat_size));
        }

        *rcl = 0;
        *roff = 0;
        *rlcl = 0;
        *rloff = 0;
        err = 0;
        goto out;
    }

    if(fs->sb.fs_type == FAT_FS_FAT32) {
        cl = fs->sb.root_dir;
    }
    else {
        cl = 0x80000000 | (fs->sb.reserved_sectors + (fs->sb.num_fats *
            fs->sb.fat_size));
    }

    if(is_component_short(tok)) {
        normalize_shortname(tok, comp);

        if((err = fat_search_dir(fs, comp, cl, &cur, &cl, &off)) < 0)
            goto out;
    }
    else {
        if((err = fat_search_long(fs, tok, cl, &cur, &cl, &off, &lcl,
                                  &loff)) < 0)
            goto out;
    }

    tok = strtok_r(NULL, "/", &tmp);

    while(tok) {
        /* Make sure what we're looking at is a directory... */
        if(!(cur.attr & FAT_ATTR_DIRECTORY)) {
            err = -ENOTDIR;
            goto out;
        }

        cl = cur.cluster_low | (cur.cluster_high << 16);

        if(is_component_short(tok)) {
            normalize_shortname(tok, comp);
            lcl = loff = 0;

            if((err = fat_search_dir(fs, comp, cl, &cur, &cl, &off)) < 0) {
                goto out;
            }
        }
        else {
            if((err = fat_search_long(fs, tok, cl, &cur, &cl, &off, &lcl,
                                      &loff)) < 0)
                goto out;
        }

        tok = strtok_r(NULL, "/", &tmp);
    }

    /* One last check... If the filename the user passed in ends with a '/'
       character, make sure we actually have a directory... */
    if(fn[strlen(fn) - 1] == '/' && !(cur.attr & FAT_ATTR_DIRECTORY)) {
        err = -ENOTDIR;
        goto out;
    }

    *rv = cur;
    *rcl = cl;
    *roff = off;
    *rlcl = lcl;
    *rloff = loff;

out:
    free(fnc);
    return err;
}

int fat_erase_dentry(fat_fs_t *fs, uint32_t cl, uint32_t off, uint32_t lcl,
                     uint32_t loff) {
    fat_dentry_t *ent;
    uint8_t *buf;
    uint32_t max, max2, i;
    int done = 0, err;

    /* Read the cluster/block where the short name lives. */
    if(!(buf = fat_cluster_read(fs, cl, &err))) {
        dbglog(DBG_ERROR, "Error reading directory entry at cluster %" PRIu32
               ", offset %" PRIu32 ": %s\n", cl, off, strerror(err));
        return -EIO;
    }

    /* Mark the short entry as free. */
    ent = (fat_dentry_t *)(buf + off);
    ent->name[0] = FAT_ENTRY_FREE;

    fat_cluster_mark_dirty(fs, cl);

    /* If there is a long name chain, mark it all as free too... */
    if(lcl) {
        /* Figure out how many directory entries there are in each
           cluster/block. */
        if(fs->sb.fs_type == FAT_FS_FAT32 || !(lcl & 0x80000000)) {
            /* Either we're working with a regular directory or we're working
               with the root directory on FAT32 (which is the same as a normal
               directory). We care about the number of entries per cluster. */
            max = (fs->sb.bytes_per_sector * fs->sb.sectors_per_cluster) >> 5;
            max2 = 0xFFFFFFFF;
        }
        else {
            /* We're working with the root directory on FAT12/FAT16, so we need
               to look at the number of entries per sector. */
            max = fs->sb.bytes_per_sector >> 5;
            max2 = (int32_t)fs->sb.root_dir;
        }

        i = loff >> 5;

        while(!done) {
            if(!(buf = fat_cluster_read(fs, lcl, &err))) {
                dbglog(DBG_ERROR, "Error reading directory entry at cluster %"
                       PRIu32 ", offst %" PRIu32 ": %s\n", lcl, loff,
                       strerror(err));
                return -EIO;
            }

            /* Just go ahead and do this now to save us the trouble later... */
            fat_cluster_mark_dirty(fs, lcl);

            for(; i < max; ++i) {
                ent = (fat_dentry_t *)(buf + (i << 5));

                /* If name[0] is zero, then we've hit the end of the
                   directory. */
                if(ent->name[0] == FAT_ENTRY_EOD) {
                    dbglog(DBG_ERROR, "End of directory hit while reading long "
                           "name entry for deletion at cluster %" PRIu32
                           ", offset %" PRIu32 "\n", lcl, i << 5);
                    return -EIO;
                }
                /* If name[0] == 0xE5, then this entry is empty. We previously
                   marked the short name entry (which should be right after the
                   long name chain) as empty, so this means we have finished
                   deleting the long name.  */
                else if(ent->name[0] == FAT_ENTRY_FREE) {
                    done = 1;
                    break;
                }
                /* We shouldn't have any entries that aren't long names in the
                   middle of a long name chain... */
                else if(!FAT_IS_LONG_NAME(ent)) {
                    dbglog(DBG_ERROR, "Invalid dentry hit while reading long "
                           "name entry for deletion at cluster %" PRIu32
                           ", offset %" PRIu32 "\n", lcl, i << 5);
                    return -EIO;
                }

                /* Mark the entry as empty and move on... */
                ent->name[0] = FAT_ENTRY_FREE;
            }

	    if(!done) {
                /* Move onto the next cluster. */
                if(!(lcl & 0x80000000)) {
                    lcl = fat_read_fat(fs, lcl, &err);
                    if(lcl == 0xFFFFFFFF) {
                        dbglog(DBG_ERROR, "Invalid FAT value hit while "
			       "reading long name entry for deletion at "
			       "cluster %" PRIu32 ", offset %" PRIu32 "\n",
			       lcl, i << 5);
                        return -err;
                    }

                    /* This shouldn't happen either... */
                    if(fat_is_eof(fs, lcl)) {
                        dbglog(DBG_ERROR, "End of directory hit while "
			       "reading long name entry for deletion at "
			       "cluster %" PRIu32 ", offset %" PRIu32 "\n",
			       lcl, i << 5);
                        return -EIO;
                    }

                    i = 0;
                }
                else {
                    ++lcl;
                    max2 -= max;

                    if(max2 <= 0) {
                        dbglog(DBG_ERROR, "End of directory hit while "
			       "reading long name entry for deletion at "
			       "cluster %" PRIu32 ", offset %" PRIu32 "\n",
			       lcl, i << 5);
                        return -EIO;
                    }

                    i = 0;
                }
	    }
        }
    }

    return 0;
}

int fat_is_dir_empty(fat_fs_t *fs, uint32_t cluster) {
    uint8_t *cl;
    int err, done = 0;
    uint32_t i, j = 0, max;
    fat_dentry_t *ent;

    /* Figure out how many directory entries there are in each cluster/block. */
    if(fs->sb.fs_type == FAT_FS_FAT32 || !(cluster & 0x80000000)) {
        /* Either we're working with a regular directory or we're working with
           the root directory on FAT32 (which is the same as a normal
           directory). We care about the number of entries per cluster. */
        max = (fs->sb.bytes_per_sector * fs->sb.sectors_per_cluster) >> 5;
    }
    else {
        /* We're working with the root directory on FAT12/FAT16, so we need to
           look at the number of entries per sector. */
        max = fs->sb.bytes_per_sector >> 5;
    }

    while(!done) {
        if(!(cl = fat_cluster_read(fs, cluster, &err))) {
            dbglog(DBG_ERROR, "Error reading directory at cluster %" PRIu32
                   ": %s\n", cluster, strerror(err));
            return -EIO;
        }

        for(i = 0; i < max && !done; ++i, ++j) {
            ent = (fat_dentry_t *)(cl + (i << 5));

            /* If name[0] is zero, then we've hit the end of the directory. */
            if(ent->name[0] == FAT_ENTRY_EOD) {
                return 1;
            }
            /* If name[0] == 0xE5, then this entry is empty (but there might
               still be additional entries after it). */
            else if(ent->name[0] == FAT_ENTRY_FREE) {
                continue;
            }
            /* Ignore long name entries. */
            else if(FAT_IS_LONG_NAME(ent)) {
                continue;
            }

            /* There's only two valid entries we can have in an empty directory,
               "." and "..". */
            if(!memcmp(ent->name, DOT_NAME, 11) ||
               !memcmp(ent->name, DOTDOT_NAME, 11)) {
                continue;
            }

            /* If we find a valid short entry, other than "." or "..", then
               the directory is not empty. Return failure. */
            return 0;
        }

        if(!(cluster & 0x80000000)) {
            cluster = fat_read_fat(fs, cluster, &err);
            if(cluster == 0xFFFFFFFF)
                return -err;

            if(fat_is_eof(fs, cluster))
                done = 1;
        }
        else {
            ++cluster;

            if(j >= fs->sb.root_dir)
                done = 1;
        }
    }

    /* If we get here, we hit the end of chain marker in the FAT (or the max
       entries in the root dir of FAT12/FAT16). Thus, there's nothing in the
       directory or we would have bailed out before now. */
    return 1;
}

static int fat_get_free_dentry(fat_fs_t *fs, uint32_t cluster, uint32_t *rcl,
                               uint32_t *roff, fat_dentry_t **rv,
                               uint32_t num) {
    uint8_t *cl;
    int err, done = 0;
    uint32_t i, j = 0, max, old = cluster, ct = 0;
    fat_dentry_t *ent, *sent = NULL;
    uint32_t scl = cluster, soff = 0;

    /* Figure out how many directory entries there are in each cluster/block. */
    if(fs->sb.fs_type == FAT_FS_FAT32 || !(cluster & 0x80000000)) {
        /* Either we're working with a regular directory or we're working with
           the root directory on FAT32 (which is the same as a normal
           directory). We care about the number of entries per cluster. */
        max = (fs->sb.bytes_per_sector * fs->sb.sectors_per_cluster) >> 5;
    }
    else {
        /* We're working with the root directory on FAT12/FAT16, so we need to
           look at the number of entries per sector. */
        max = fs->sb.bytes_per_sector >> 5;
    }

    while(!done) {
        if(!(cl = fat_cluster_read(fs, cluster, &err))) {
            dbglog(DBG_ERROR, "Error reading directory at cluster %" PRIu32
                   ": %s\n", cluster, strerror(err));
            return -EIO;
        }

        for(i = 0; i < max && !done; ++i, ++j) {
            ent = (fat_dentry_t *)(cl + (i << 5));

            /* If name[0] is zero, then we've hit the end of the directory. */
            if(ent->name[0] == FAT_ENTRY_EOD) {
                ++ct;

                /* Just because we found the end of the directory doesn't mean
                   that there is necessarily space (in the case of the root
                   directory on FAT12/FAT16). */
                if((cluster & 0x80000000)) {
                    if(j + num - ct >= fs->sb.root_dir) {
                        *rv = NULL;
                        return -ENOSPC;
                    }
                }

                /* Do we have enough space left in this cluster to store the
                   entire dentry (long and short)? */
                if(max - i - ct >= num || (cluster & 0x80000000)) {
                    if(ct == 1) {
                        *rv = ent;
                        *rcl = cluster;
                        *roff = i << 5;
                    }
                    else {
                        *rv = sent;
                        *rcl = scl;
                        *roff = soff;
                    }

                    return 0;
                }
                else {
                    /* We're gonna have to allocate another cluster to finish
                       off the entry... */
                    if(ct == 1) {
                        sent = ent;
                        scl = cluster;
                        soff = i << 5;
                    }

                    goto alloc_another;
                }
            }
            /* If name[0] == 0xE5, then this entry is empty (but there might
               still be additional entries after it). */
            else if(ent->name[0] == FAT_ENTRY_FREE) {
                ++ct;

                /* If this is the first entry, set up the pointers we'll need
                   later if this turns out to be where we store the directory
                   entry. */
                if(ct == 1) {
                    sent = ent;
                    scl = cluster;
                    soff = i << 5;
                }

                /* If we've got enough entries, then we can return success
                   at this point. */
                if(ct == num) {
                    *rv = sent;
                    *rcl = scl;
                    *roff = soff;
                    return 0;
                }
            }
            /* Just ignore anything else... */
            else {
                /* Reset the counter, because we've broken the chain of free
                   entries. */
                ct = 0;
            }
        }

        if(!(cluster & 0x80000000)) {
            old = cluster;
            cluster = fat_read_fat(fs, old, &err);
            if(cluster == 0xFFFFFFFF)
                return -err;

            if(fat_is_eof(fs, cluster))
                done = 1;
        }
        else {
            ++cluster;

            if(j >= fs->sb.root_dir) {
                *rv = NULL;
                return -ENOSPC;
            }
        }
    }

    /* If we get here, that means we've run out of space in what's allocated
       for the directory (and it's not the end of the FAT12/FAT16 root).
       Attempt to allocate a new cluster, clear it out, and return a pointer to
       the beginning of it. */
alloc_another:
    if((j = fat_allocate_cluster(fs, &err)) == FAT_INVALID_CLUSTER) {
        dbglog(DBG_ERROR, "Error allocating directory cluster: %s\n",
               strerror(err));
        *rv = NULL;
        return err;
    }

    /* Update the FAT chain. */
    if((err = fat_write_fat(fs, old, j)) < 0) {
        dbglog(DBG_ERROR, "Error writing fat for new allocation: %s\n",
               strerror(err));
        fat_write_fat(fs, j, 0);
        *rv = NULL;
        return -err;
    }

    /* Clear the new block and return a pointer to the beginning of it. */
    if(!(cl = fat_cluster_clear(fs, j, &err))) {
        /* Deallocate the cluster we just allocated. */
        fat_write_fat(fs, j, 0);

        /* This will get properly truncated for FAT12/FAT16. */
        fat_write_fat(fs, old, 0x0FFFFFFF);
        *rv = NULL;
        return err;
    }

    if(ct == 0) {
        *rv = (fat_dentry_t *)cl;
        *rcl = j;
        *roff = 0;
    }
    else {
        *rv = sent;
        *rcl = scl;
        *roff = soff;
    }

    return 0;
}

static inline void fill_timestamp(struct tm *now, uint16_t *date,
                                  uint16_t *ts, uint8_t *tenth) {
    /* The MS-DOS epoch is January 1, 1980, not January 1, 1970... */
    *date = ((now->tm_year - 80) & 0x7f) << 9;
    *date |= (now->tm_mon + 1) << 5;
    *date |= now->tm_mday;

    if(ts) {
        *ts = now->tm_hour << 11;
        *ts |= now->tm_min << 5;
        *ts |= now->tm_sec >> 1;     /* two-second resolution.... */

        /* Meh, good enough. */
        if(tenth) {
            *tenth = (now->tm_sec & 1) * 100;
        }
    }
}

inline void fat_add_raw_dentry(fat_dentry_t *dent, const char shortname[11],
                               uint8_t attr, uint32_t cluster) {
    time_t now = time(NULL);
    struct tm *tmv;

    memset(dent, 0, sizeof(fat_dentry_t));
    memcpy(dent->name, shortname, 11);
    dent->attr = attr;
    dent->cluster_high = (uint16_t)(cluster >> 16);
    dent->cluster_low = (uint16_t)cluster;

    /* Fill in the timestamps... */
    tmv = localtime(&now);
    fill_timestamp(tmv, &dent->cdate, &dent->ctime, &dent->ctenth);
    fill_timestamp(tmv, &dent->mdate, &dent->mtime, NULL);
    fill_timestamp(tmv, &dent->adate, NULL, NULL);
}

static int create_shortname(fat_fs_t *fs, const char *fn, size_t fn_len,
                            char out[11], fat_dentry_t *parent) {
    char *fnc;
    char denorm[13] = { 0 };
    size_t i, j = 0, k = 0;
    int has_ext = 0, last_period = -1, found_char = 0, tail = 0, done = 0;

    if(!(fnc = (char *)malloc(fn_len + 1)))
        return -ENOMEM;

    /* Convert our copy to uppercase. */
    for(i = 0; i < fn_len;) {
        /* Skip non-usable characters. */
        if(fn[i] <= ' ') {
            ++i;
        }
        /* These should've already been caught. */
        else if(fn[i] == '*' || fn[i] == ':' || fn[i] == '/' || fn[i] == '\\' ||
                fn[i] == '|' || fn[i] == '"' || fn[i] == '?' || fn[i] == '<' ||
                fn[i] == '>') {
            free(fnc);
            return -EILSEQ;
        }
        /* Convert characters that can't be used in a short name to '_'. */
        else if(fn[i] == '+' || fn[i] == ',' || fn[i] == ';' || fn[i] == '[' ||
                fn[i] == ']' || fn[i] == '=') {
            fnc[j++] = '_';
            found_char = 1;
            ++i;

            if(has_ext == 1)
                has_ext = 2;
        }
        else if(fn[i] == '.') {
            if(found_char) {
                has_ext = 1;
                last_period = j;
                fnc[j++] = '.';
            }
            ++i;
        }
        /* Convert other normal ASCII to uppercase. */
        else if((uint8_t)fn[i] <= 0x7F) {
            fnc[j++] = toupper((int)fn[i++]);
            found_char = 1;

            if(has_ext == 1)
                has_ext = 2;
        }
        /* Do we have a 2 byte UTF-8 sequence? */
        else if((fn[i] & 0xE0) == 0xC0) {
            fnc[j++] = '_';
            i += 2;
            found_char = 1;

            if(has_ext == 1)
                has_ext = 2;
        }
        /* Do we have a 3 UTF-8 byte sequence? */
        else if((fn[i] & 0xF0) == 0xE0) {
            fnc[j++] = '_';
            i += 3;
            found_char = 1;

            if(has_ext == 1)
                has_ext = 2;
        }
        /* 4 byte UTF-8 sequences can't be encoded as UCS-2. */
        else {
            free(fnc);
            return -EILSEQ;
        }
    }

    /* Terminate the end of the copied string. */
    fnc[j] = 0;

    /* Now we create the basis name. */
    for(i = 0; k < 8 && i < j && i < (size_t)last_period; ++i) {
        if(fnc[i] != '.')
            denorm[k++] = fnc[i];
    }

    if(has_ext) {
        denorm[k++] = '.';

        for(i = 0; i < 3 && i + last_period + 1 < j; ++i) {
            denorm[k++] = fnc[i + last_period + 1];
        }
    }

    /* We're done with this now. */
    free(fnc);

    /* Next up is generating a numeric tail and seeingif the file already
       exists in the directory. */
    normalize_shortname(denorm, out);

    while(!done) {
        ++tail;

        if(tail < 10) {
            out[6] = '~';
            out[7] = '0' + tail;
        }
        else if(tail < 100) {
            out[5] = '~';
            out[6] = '0' + (tail / 10);
            out[7] = '0' + (tail % 10);
        }
        /* God help us if we ever really get below that one above... */
        else if(tail < 1000) {
            out[4] = '~';
            out[5] = '0' + (tail / 100);
            out[6] = '0' + ((tail / 10) % 10);
            out[7] = '0' + (tail % 10);
        }
        else if(tail < 10000) {
            out[3] = '~';
            out[4] = '0' + (tail / 1000);
            out[5] = '0' + ((tail / 100) % 10);
            out[6] = '0' + ((tail / 10) % 10);
            out[7] = '0' + (tail % 10);
        }
        else if(tail < 100000) {
            out[2] = '~';
            out[3] = '0' + (tail / 10000);
            out[4] = '0' + ((tail / 1000) % 10);
            out[5] = '0' + ((tail / 100) % 10);
            out[6] = '0' + ((tail / 10) % 10);
            out[7] = '0' + (tail % 10);
        }
        else if(tail < 1000000) {
            out[1] = '~';
            out[2] = '0' + (tail / 100000);
            out[3] = '0' + ((tail / 10000) % 10);
            out[4] = '0' + ((tail / 1000) % 10);
            out[5] = '0' + ((tail / 100) % 10);
            out[6] = '0' + ((tail / 10) % 10);
            out[7] = '0' + (tail % 10);
        }
        else {
            /* This really should never happen... There's not really a valid
               way for this to ever happen... */
            return -ENOSPC;
        }

        /* See if the filename exists that we're trying to use. */
        if(fat_find_child2(fs, out, parent) == -ENOENT) {
            done = 1;
        }
    }

    return 0;
}

int fat_add_long_entry(fat_fs_t *fs, char sn[11], uint32_t dents,
                       uint8_t attr, uint32_t cluster, uint32_t *rcl,
                       uint32_t *roff, uint32_t rlcl, uint32_t rloff,
                       uint8_t cs) {
    fat_longname_t *ent;
    fat_dentry_t *ent2;
    int pos = (dents - 1) * 13, done = 0, err = 0;
    uint32_t i, j = dents, old;
    size_t max;
    uint8_t *cl;

    /* Figure out how many directory entries there are in each cluster/block. */
    if(fs->sb.fs_type == FAT_FS_FAT32 || !(rlcl & 0x80000000)) {
        /* Either we're working with a regular directory or we're working with
           the root directory on FAT32 (which is the same as a normal
           directory). We care about the number of entries per cluster. */
        max = (fs->sb.bytes_per_sector * fs->sb.sectors_per_cluster);
    }
    else {
        /* We're working with the root directory on FAT12/FAT16, so we need to
           look at the number of entries per sector. */
        max = fs->sb.bytes_per_sector;
    }

    while(!done) {
        if(!(cl = fat_cluster_read(fs, rlcl, &err))) {
            dbglog(DBG_ERROR, "Error reading directory at cluster %" PRIu32
                   ": %s\n", cluster, strerror(err));
            return -EIO;
        }

        /* Save some trouble later... */
        fat_cluster_mark_dirty(fs, rlcl);

        /* Start building the long name directory entries from the end of the
           name to the start... */
        for(i = rloff; i < max && j; i += 32, --j, pos -= 13) {
            ent = (fat_longname_t *)(cl + i);

            memset(ent, 0, sizeof(fat_longname_t));
            ent->attr = FAT_ATTR_LONG_NAME;
            ent->checksum = cs;
            if(j == dents)
                ent->order = FAT_ORDER_LAST | dents;
            else
                ent->order = j;

            memcpy(ent->name1, &longname_buf2[pos], 10);
            memcpy(ent->name2, &longname_buf2[pos + 5], 12);
            memcpy(ent->name3, &longname_buf2[pos + 11], 4);
        }

        /* Did we finish with the long entry with space left to spare in the
           current block/cluster? If so, drop the short name here. Otherwise,
           we'll loop back around and hit this code on the next block/cluster
           pass... */
        if(!j && i < max) {
            ent2 = (fat_dentry_t *)(cl + i);

            /* Fill it in. */
            fat_add_raw_dentry(ent2, sn, attr, cluster);
            done = 1;
            *rcl = rlcl;
            *roff = i;
        }

        if(!done && !(rlcl & 0x80000000)) {
            old = rlcl;
            rlcl = fat_read_fat(fs, old, &err);
            if(rlcl == 0xFFFFFFFF)
                return -err;

            if(fat_is_eof(fs, rlcl))
                done = 1;
        }
        else {
            ++rlcl;

            if(j >= fs->sb.root_dir)
                return -ENOSPC;
        }

        rloff = 0;
    }

    return 0;
}

int fat_add_dentry(fat_fs_t *fs, const char *fn, fat_dentry_t *parent,
                   uint8_t attr, uint32_t cluster, uint32_t *rcl,
                   uint32_t *roff, uint32_t *rlcl, uint32_t *rloff) {
    fat_dentry_t *dent;
    int err;
    uint32_t cl;
    char comp[11];
    size_t len, len2;
    int dents;
    uint8_t cs;

    cl = parent->cluster_low | (parent->cluster_high << 16);

    if(is_component_short(fn)) {
        normalize_shortname(fn, comp);

        if((err = fat_get_free_dentry(fs, cl, rcl, roff, &dent, 1)) < 0)
            return err;

        /* Fill it in. */
        fat_add_raw_dentry(dent, comp, attr, cluster);

        /* Clean up. */
        *rlcl = 0;
        *rloff = 0;
        fat_cluster_mark_dirty(fs, *rcl);
        return 0;
    }
    else {
        /* Not exact, but good enough. */
        if((len2 = strlen(fn)) > 255)
            return -ENAMETOOLONG;

        /* Convert the filename to UCS-2 first. */
        if(fat_utf8_to_ucs2(longname_buf2, (const uint8_t *)fn, 256,
                            strlen(fn)) < 0) {
            return -EILSEQ;
        }

        /* Figure out how long it is in UCS-2 codepoints. */
        len = fat_strlen_ucs2(longname_buf2);

        /* Figure out how many directory entries we're gonna need. */
        dents = len / 13;

        /* Make things easier later... */
        if(len % 13) {
            ++dents;
            memset(longname_buf2 + len, 0, 13 * sizeof(uint16_t));
        }

        /* Come up with the short name the file will have. */
        if((err = create_shortname(fs, fn, len2, comp, parent)) < 0)
            return err;

        /* Calculate the checksum of the short filename. */
        cs = fat_shortname_checksum(comp);

        /* Find a contiguous place in the directory to put our long entries and
           the short entry. */
        if((err = fat_get_free_dentry(fs, cl, rlcl, rloff, &dent,
                                      dents + 1)) < 0)
            return err;

        /* Attempt to add the filename... */
        if((err = fat_add_long_entry(fs, comp, dents, attr, cluster, rcl,
                                     roff, *rlcl, *rloff, cs)))
            return err;

        return 0;
    }
}

int fat_get_dentry(fat_fs_t *fs, uint32_t cluster, uint32_t off,
                   fat_dentry_t *rv) {
    uint8_t *cl;
    int err;

    if(!(cl = fat_cluster_read(fs, cluster, &err))) {
        dbglog(DBG_ERROR, "Error reading directory at cluster %" PRIu32
               ": %s\n", cluster, strerror(err));
        return -EIO;
    }

    memcpy(rv, cl + off, sizeof(fat_dentry_t));
    return 0;
}

void fat_update_mtime(fat_dentry_t *ent) {
    time_t now = time(NULL);
    struct tm *tmv;

    tmv = localtime(&now);
    fill_timestamp(tmv, &ent->mdate, &ent->mtime, NULL);
}

int fat_update_dentry(fat_fs_t *fs, fat_dentry_t *ent, uint32_t cluster,
                      uint32_t off) {
    uint8_t *cl;
    int err;

    if(!(cl = fat_cluster_read(fs, cluster, &err))) {
        dbglog(DBG_ERROR, "Error reading directory at cluster %" PRIu32
               ": %s\n", cluster, strerror(err));
        return -EIO;
    }

    memcpy(cl + off, ent, sizeof(fat_dentry_t));
    fat_cluster_mark_dirty(fs, cluster);
    return 0;
}

#ifdef FAT_DEBUG
void fat_dentry_print(const fat_dentry_t *ent) {
    uint32_t cl = ent->cluster_low | (ent->cluster_high << 16);

    dbglog(DBG_KDEBUG, "Filename: %.11s\n", ent->name);
    dbglog(DBG_KDEBUG, "Attributes: %02x\n", ent->attr);
    dbglog(DBG_KDEBUG, "Cluster: %" PRIu32 "\n", cl);
    dbglog(DBG_KDEBUG, "Size: %" PRIu32 "\n", ent->size);
}
#endif

#if __GNUC__ >= 9
#pragma GCC diagnostic pop
#endif
