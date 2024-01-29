/* KallistiOS ##version##

   isotest.c
   (c)2000 Megan Potter

   Test ISO filesystem reader. This is a functional duplicate of fs_iso9660, but
   designed to run on a PC for testing.

*/

/*
Various notes

In Linux, accessing the /dev device on a single-session CD puts the
"bootstrap" zone at offset 0x8000, and the begin of the CD data itself at
0x8800.

*/

/****************************** LINUX SPECIFIC CODE ***********************************/

#include <stdio.h>
#include <ctype.h>

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;
typedef signed char int8;
typedef signed short int16;
typedef signed long int32;

/* Low-level sector read (for Linux to emulate hardware/cd.c) */
static int cdrom_read_sectors(void *buffer, uint32 sector, uint32 cnt) {
    FILE *f;

    /* Subtract out DC's LBA offset */
    sector -= 150;

    f = fopen("/dev/scd0", "r");

    if(!f) return -1;

    fseek(f, sector * 2048, SEEK_SET);
    if(fread(buffer, cnt * 2048, 1, f) != 1) {
        fclose(f);
        return 1;
    }

    fclose(f);
    return 0;
}

/* Linux emulation of various other KOS CD prims */
typedef int CDROM_TOC;

static int cdrom_reinit() {
    return 0;
}

static int cdrom_read_toc(CDROM_TOC *toc, int session) {
    return 0;
}

static uint32 cdrom_locate_data_track(CDROM_TOC *toc) {
    return 150;
}

/* KOS VFS prims */
#define O_RDONLY 0
#define O_MODE_MASK 0xfff
#define O_DIR 0x1000
#define MAX_FN_LEN 256
typedef struct {
    int     size;
    char    name[MAX_FN_LEN];
    uint32  time;
    uint32  attr;
} dirent_t;

typedef void *vfs_handler[];

static int fs_handler_add(char *fn, void *p) {
    return 0;
}

static int fs_handler_remove(void *p) {
    return 0;
}

/* Thread prims */
typedef int thd_mutex_t;

static void thd_mutex_reset(int *p) { }
static void thd_mutex_lock(int *p) { }
static void thd_mutex_unlock(int *p) { }

/* iso9660 defines */
#define MAX_ISO_FILES 8

/****************************** END LINUX SPECIFIC CODE ***********************************/

/* Cut here to insert into KallistiOS fs_iso9660.c */


/* #include <kallisti/stdtypes.h> */
#include <stdlib.h>
#include <string.h>

/********************************************************************************/
/* Low-level ISO utils */

/* ISO Directory entry */
typedef struct {
    uint8   length;         /* 711 */
    uint8   ext_attr_length;    /* 711 */
    uint8   extent[8];      /* 733 */
    uint8   size[8];        /* 733 */
    uint8   date[7];        /* 7x711 */
    uint8   flags;
    uint8   file_unit_size;     /* 711 */
    uint8   interleave;     /* 711 */
    uint8   vol_sequence[4];    /* 723 */
    uint8   name_len;       /* 711 */
    char    name[1];
} iso_dirent_t;

#if 0
/* Util function to reverse the byte order of a uint32 */
static uint32 ntohl_32(void *data) {
    uint8 *d = (uint8*)data;
    return (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | (d[3] << 0);
}
#endif

/* This seems kinda silly, but it's important since it allows us
   to do unaligned accesses on a buffer */
static uint32 htohl_32(void *data) {
    uint8 *d = (uint8*)data;
    return (d[0] << 0) | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
}

#if 0
/* Read red-book section 7.1.1 number (8 bit) */
static uint8 iso_711(uint8 *from) {
    return (*from & 0xff);
}
#endif

/* Read red-book section 7.3.3 number (32 bit LE / 32 bit BE) */
static uint32 iso_733(uint8 *from) {
    return htohl_32(from);
}


/********************************************************************************/
/* Low-level block caching routines. This implements a simple queue-based
   LRU/MRU caching system. Whenever a block is requested, it will be placed
   on the MRU end of the queue. As more blocks are loaded than can fit in
   the cache, blocks are deleted from the LRU end. */

/* Holds the data for one cache block, and a pointer to the next one.
   As sectors are read from the disc, they are added to the front of
   this cache. As the cache fills up, sectors are removed from the end
   of it. */
typedef struct {
    int32   sector;         /* CD sector */
    uint8   data[2048];     /* Sector data */
} cache_block_t;

/* List of cache blocks (ordered least recently used to most recently) */
#define NUM_CACHE_BLOCKS 8
static cache_block_t *cache[NUM_CACHE_BLOCKS];

/* Cache modification mutex */
static thd_mutex_t cache_mutex;

/* Clears all cache blocks */
static void bclear() {
    int i;

    thd_mutex_lock(&cache_mutex);

    for(i = 0; i < NUM_CACHE_BLOCKS; i++)
        cache[i]->sector = -1;

    thd_mutex_unlock(&cache_mutex);
}

/* Graduate a block from its current position to the MRU end of the cache */
static void bgrad(int block) {
    int     i;
    cache_block_t   *tmp;

    /* Don't try it with the end block */
    if(block < 0 || block >= (NUM_CACHE_BLOCKS - 1)) return;

    /* Make a copy and scoot everything down */
    tmp = cache[block];

    for(i = block; i < (NUM_CACHE_BLOCKS - 1); i++)
        cache[i] = cache[i + 1];

    cache[NUM_CACHE_BLOCKS - 1] = tmp;
}

/* Pulls the requested sector into a cache block and returns the cache
   block index. Note that the sector in question may already be in the
   cache, in which case it just returns the containing block. */
static int bread(int32 sector) {
    int i, rv = -1;

    thd_mutex_lock(&cache_mutex);

    /* Look for a pre-existing cache block */
    for(i = NUM_CACHE_BLOCKS - 1; i >= 0; i--) {
        if(cache[i]->sector == sector) {
            bgrad(i);
            rv = NUM_CACHE_BLOCKS - 1;
            goto bread_exit;
        }
    }

    /* If not, look for an open cache slot; if we find one, use it */
    for(i = 0; i < NUM_CACHE_BLOCKS; i++) {
        if(cache[i]->sector == -1) break;
    }

    /* If we didn't find one, kick an LRU block out of cache */
    if(i >= NUM_CACHE_BLOCKS) {
        i = 0;
    }

    /* Load the requested block */
    if(cdrom_read_sectors(cache[i]->data, sector + 150, 1) < 0) {
        rv = -1;
        goto bread_exit;
    }

    cache[i]->sector = sector;

    /* Move it to the most-recently-used position */
    bgrad(i);
    rv = NUM_CACHE_BLOCKS - 1;

    /* Return the new cache block index */
bread_exit:
    thd_mutex_unlock(&cache_mutex);
    return rv;
}


/********************************************************************************/
/* Higher-level ISO9660 primitives */

/* Root FS session location (in sectors) */
static int32 session_base = 0;

/* Root directory extent and size in bytes */
static int32 root_extent = 0, root_size = 0;

/* Root dirent */
static iso_dirent_t root_dirent;


/* Per-disc initialization; this is done every time it's discovered that
   a new CD has been inserted. */
static int init_percd() {
    int     i;
    CDROM_TOC   toc;

    /* Start off with no cached blocks */
    bclear();

    /* Locate the root session */
    if((i = cdrom_reinit()) != 0)
        return i;

    if((i = cdrom_read_toc(&toc, 0)) != 0)
        return i;

    if(!(session_base = cdrom_locate_data_track(&toc)))
        return -1;

    /* Grab and check the volume descriptor */
    i = bread(session_base + 16 - 150);

    if(i < 0) return i;

    if(memcmp((char*)cache[i]->data, "\01CD001", 6)) {
        printf("fs_iso9660: disc is not iso9660\r\n");
        return -1;
    }

    /* Locate the root directory */
    memcpy(&root_dirent, cache[i]->data + 156, sizeof(iso_dirent_t));
    root_extent = iso_733(root_dirent.extent);
    root_size = iso_733(root_dirent.size);

    return 0;
}

/* Compare an ISO9660 filename against a normal filename. This takes into
   account the version code on the end and is not case sensitive. */
static int fncompare(const char *isofn, int isosize, const char *normalfn) {
    int i;

    for(i = 0; i < isosize; i++) {
        if(isofn[i] == ';') return 0;

        if(tolower(isofn[i]) != tolower(normalfn[i]))
            return -1;
    }

    return 0;
}

/* Locate an ISO9660 object in the given directory; this can be a directory or
   a file, it works fine for either one. Pass in:

   fn:      object filename (relative to the passed directory)
   dir:     0 if looking for a file, 1 if looking for a dir
   dir_extent:  directory extent to start with
   dir_size:    directory size (in bytes)

   It will return a pointer to a transient dirent buffer (i.e., don't
   expect this buffer to stay around much longer than the call itself).
 */
static iso_dirent_t *find_object(const char *fn, int dir,
                                 int32 dir_extent, int32 dir_size) {
    int32         i;
    iso_dirent_t * de;

    while(dir_size > 0) {
        int c = bread(dir_extent);

        if(c < 0) return NULL;

        for(i = 0; i < 2048 && i < dir_size;) {
            /* Locate the current dirent */
            de = (iso_dirent_t *)(cache[c]->data + i);

            if(!de->length) break;

            /* Check the filename against the requested one */
            if(!fncompare(de->name, de->name_len, fn)) {
                if(!((dir << 1) ^ de->flags))
                    return de;
            }

            i += de->length;
        }

        dir_extent++;
        dir_size -= 2048;
    }

    return NULL;
}

/* Locate an ISO9660 object anywhere on the disc, starting at the root,
   and expecting a fully qualified path name. This is analogous to find_object
   but it searches with the path in mind.

   fn:      object filename (relative to the passed directory)
   dir:     0 if looking for a file, 1 if looking for a dir
   dir_extent:  directory extent to start with
   dir_size:    directory size (in bytes)

   It will return a pointer to a transient dirent buffer (i.e., don't
   expect this buffer to stay around much longer than the call itself).
 */
static iso_dirent_t *find_object_path(const char *fn, int dir, iso_dirent_t *start) {
    char        *cur;

    /* If the object is in a sub-tree, traverse the trees looking
       for the right directory */
    while((cur = strchr(fn, '/'))) {
        if(cur != fn) {
            /* Note: trailing path parts don't matter since find_object
               only compares based on the FN length on the disc. */
            start = find_object(fn, 1, iso_733(start->extent), iso_733(start->size));

            if(start == NULL) return NULL;
        }

        fn = cur + 1;
    }

    /* Locate the file in the resulting directory */
    if(*fn) {
        start = find_object(fn, dir, iso_733(start->extent), iso_733(start->size));
        return start;
    }
    else {
        if(!dir)
            return NULL;
        else
            return start;
    }
}

/********************************************************************************/
/* File primitives */

/* File handles.. I could probably do this with a linked list, but I'm just
   too lazy right now. =) */
static struct {
    int32      first_extent;   /* First sector */
    int     dir;        /* >0 if a directory */
    int32      ptr;        /* Current read position in bytes */
    int32      size;       /* Length of file in bytes */
    dirent_t    dirent;     /* A static dirent to pass back to clients */
} fh[MAX_ISO_FILES];

/* Mutex for file handles */
static thd_mutex_t fh_mutex;

/* Open a file or directory */
static int32 iso_open(const char *fn, int mode) {
    int32      fd;
    iso_dirent_t    *de;

    /* Make sure they don't want to open things as writeable */
    if((mode & O_MODE_MASK) != O_RDONLY)
        return 0;

    /* Find a free file handle */
    thd_mutex_lock(&fh_mutex);

    for(fd = 0; fd < MAX_ISO_FILES; fd++)
        if(fh[fd].first_extent == 0) {
            fh[fd].first_extent = -1;
            break;
        }

    thd_mutex_unlock(&fh_mutex);

    if(fd >= MAX_ISO_FILES)
        return 0;

    /* For now we always do this, but it should eventually be set
       up to detect the CD tray having been opened. */
    if(init_percd() < 0)
        return 0;

    /* Find the file we want */
    de = find_object_path(fn, (mode & O_DIR) ? 1 : 0, &root_dirent);

    if(!de) return 0;

    /* Fill in the file handle and return the fd */
    fh[fd].first_extent = iso_733(de->extent);
    fh[fd].dir = (mode & O_DIR) ? 1 : 0;
    fh[fd].ptr = 0;
    fh[fd].size = iso_733(de->size);

    return fd;
}

/* Close a file or directory */
static void iso_close(uint32 fd) {
    /* Check that the fd is valid */
    if(fd < MAX_ISO_FILES) {
        /* No need to lock the mutex: this is an atomic op */
        fh[fd].first_extent = 0;
    }
}

/* Read from a file */
static ssize_t iso_read(uint32 fd, char *buf, size_t bytes) {
    int rv = 0, c;
    size_t toread, thissect;

    /* Check that the fd is valid */
    if(fd >= MAX_ISO_FILES || fh[fd].first_extent == 0)
        return -1;

    /* Read zero or more sectors into the buffer from the current pos */
    while(bytes > 0) {
        /* Figure out how much we still need to read */
        toread = (bytes > (size_t)(fh[fd].size - fh[fd].ptr)) ?
                 (size_t)(fh[fd].size - fh[fd].ptr) : bytes;

        if(toread == 0) break;

        /* How much more can we read in the current sector? */
        thissect = 2048 - (fh[fd].ptr % 2048);
        toread = (toread > thissect) ? thissect : toread;

        /* Do the read */
        c = bread(fh[fd].first_extent + fh[fd].ptr / 2048);

        if(c < 0) return -1;

        memcpy(buf, cache[c]->data + (fh[fd].ptr % 2048), toread);

        /* Adjust pointers */
        buf += toread;
        fh[fd].ptr += toread;
        bytes -= toread;
        rv += toread;
    }

    return rv;
}

/* Seek elsewhere in a file */
static off_t iso_seek(uint32 fd, off_t offset, int whence) {
    /* Check that the fd is valid */
    if(fd >= MAX_ISO_FILES || fh[fd].first_extent == 0)
        return -1;

    /* Update current position according to arguments */
    switch(whence) {
        case SEEK_SET:
            fh[fd].ptr = offset;
        case SEEK_CUR:
            fh[fd].ptr += offset;
        case SEEK_END:
            fh[fd].ptr = fh[fd].size + offset;
        default:
            return -1;
    }

    /* Check bounds */
    if(fh[fd].ptr > fh[fd].size) fh[fd].ptr = fh[fd].size;

    return fh[fd].ptr;
}

/* Tell where in the file we are */
static off_t iso_tell(uint32 fd) {
    if(fd >= MAX_ISO_FILES || fh[fd].first_extent == 0)
        return -1;

    return fh[fd].ptr;
}

/* Tell how big the file is */
static size_t iso_total(uint32 fd) {
    if(fd >= MAX_ISO_FILES || fh[fd].first_extent == 0)
        return -1;

    return fh[fd].size;
}

/* Helper function for readdir: post-processes an ISO filename to make
   it a bit prettier. */
static void fn_postprocess(char *fn) {
    while(*fn && *fn != ';') {
        *fn = tolower(*fn);
        fn++;
    }

    *fn = 0;
}

/* Read a directory entry */
static dirent_t *iso_readdir(uint32 fd) {
    int     c;
    iso_dirent_t    *de;

    if(fd >= MAX_ISO_FILES || fh[fd].first_extent == 0 || !fh[fd].dir)
        return NULL;

    /* Scan forwards until we find the next valid entry, an
       end-of-entry mark, or run out of dir size. */
    while(fh[fd].ptr < fh[fd].size) {
        /* Get the current dirent block */
        c = bread(fh[fd].first_extent + fh[fd].ptr / 2048);

        if(c < 0) return NULL;

        de = (iso_dirent_t *)(cache[c]->data + (fh[fd].ptr % 2048));

        if(de->length) break;

        /* Skip to the next sector */
        fh[fd].ptr += 2048 - (fh[fd].ptr % 2048);
    }

    if(fh[fd].ptr >= fh[fd].size) return NULL;

    /* If we're at the first, skip the two blank entries */
    if(!de->name[0]) {
        fh[fd].ptr += de->length;
        de = (iso_dirent_t *)(cache[c]->data + (fh[fd].ptr % 2048));
        fh[fd].ptr += de->length;
        de = (iso_dirent_t *)(cache[c]->data + (fh[fd].ptr % 2048));

        if(!de->length) return NULL;
    }

    /* Fill out the VFS dirent */
    strncpy(fh[fd].dirent.name, de->name, de->name_len);
    fh[fd].dirent.name[de->name_len] = 0;
    fn_postprocess(fh[fd].dirent.name);

    if(de->flags & 2)
        fh[fd].dirent.size = -1;
    else
        fh[fd].dirent.size = iso_733(de->size);

    fh[fd].ptr += de->length;

    return &fh[fd].dirent;
}

/* Put everything together */
static vfs_handler vh = {
    0, 0,       /* In-kernel, no caching */
    iso_open,
    iso_close,
    iso_read,
    NULL,
    iso_seek,
    iso_tell,
    iso_total,
    iso_readdir,
    NULL,
    NULL
};

/* Initialize the file system */
static int fs_iso9660_init() {
    int i;

    /* Reset fd's */
    memset(fh, 0, sizeof(fh));

    /* Mark the first as active so we can have an error FD of zero */
    fh[0].first_extent = -1;

    /* Init thread mutexes */
    thd_mutex_reset(&cache_mutex);
    thd_mutex_reset(&fh_mutex);

    /* Allocate cache block space */
    for(i = 0; i < NUM_CACHE_BLOCKS; i++) {
        cache[i] = malloc(sizeof(cache_block_t));
        cache[i]->sector = -1;
    }

    /* Register with VFS */
    return fs_handler_add("/cd", &vh);
}

/* De-init the file system */
static int fs_iso9660_shutdown() {
    int i;

    /* Dealloc cache block space */
    for(i = 0; i < NUM_CACHE_BLOCKS; i++)
        free(cache[i]);

    return fs_handler_remove(&vh);
}



/* Cut here to insert into KallistiOS fs_iso9660.c */

/********************************************************************************/

#if 0
static void read_dir(uint32 extent, uint32 size) {
    int     i;
    iso_dirent_t    *de;
    char        fn[32];

    while(size > 0) {
        int c = bread(extent);

        if(c < 0) return;

        for(i = 0; i < 2048 && i < size;) {
            de = (iso_dirent_t*)(cache[c]->data + i);

            if(!de->length) break;

            strncpy(fn, de->name, de->name_len);
            fn[de->name_len] = 0;

            if(de->flags & 2)
                printf("%s\t\t<DIR>\n", fn);
            else
                printf("%s\t\t%d\n", fn, iso_733(de->size));

            i += de->length;
        }

        extent++;
        size -= 2048;
    }
}
#endif


int main() {
    fs_iso9660_init();

    /*
    {
    uint32  fd, size, t;
    char    buf[667];

    printf("Opening file /escape.txt\n");
    fd = iso_open("/escape.txt", O_RDONLY);
    if (fd == 0) {
        printf("Couldn't open file\n");
        return 1;
    }
    size = iso_total(fd);
    printf("fd is %d, size is %08lx\n", fd, size);

    while (size > 0) {
        int r;
        r = iso_read(fd, buf, 666);
        if (r < 0) {
            printf("Read error\n");
            return 1;
        }
        buf[r] = 0;
        printf("%s", buf);
        if (strstr(buf, "Let the party")) {
            t = 0;
        }
        size -= r;
    }
    printf("\nFile read is done\n");
    iso_close(fd); */

    {
        uint32      fd;
        dirent_t    *de;

        printf("Opening /demos");
        fd = iso_open("/music/mc2/entries/rookie", O_RDONLY | O_DIR);

        if(fd == 0) {
            printf("Couldn't open file\n");
            return 1;
        }

        printf("Scanning dir:\n");

        while((de = iso_readdir(fd))) {
            printf("%s\t%d\n", de->name, de->size);

            if(!strcmp(de->name, "R_WAR.ZIP")) {
                /* what's this test for? */
            }
        }

        iso_close(fd);
    }

    fs_iso9660_shutdown();

    return 0;
}
