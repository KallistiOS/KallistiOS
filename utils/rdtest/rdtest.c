/* KallistiOS ##version##

   rdtest.c
   (c)2001 Megan Potter

   Test romdisk filesystem reader. This is a functional duplicate of fs_romdisk, but
   designed to run on a PC for testing.

*/

/****************************** LINUX SPECIFIC CODE ***********************************/

#include <stdio.h>

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;
typedef signed char int8;
typedef signed short int16;
typedef signed long int32;

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

/* romdisk defines */
#define MAX_RD_FILES 8

/****************************** END LINUX SPECIFIC CODE ***********************************/

/* Cut here to insert into KallistiOS fs_romdisk.c */


/* #include <kallisti/stdtypes.h> */
#include <stdlib.h>
#include <string.h>

/* Header definitions from Linux ROMFS documentation; all integer quantities are
   expressed in big-endian notation. Unfortunately the ROMFS guys were being
   clever and made this header a variable length depending on the size of
   the volume name *groan*. Its size will be a multiple of 16 bytes though. */
typedef struct {
    char    magic[8];       /* Should be "-rom1fs-" */
    uint32  full_size;      /* Full size of the file system */
    uint32  checksum;       /* Checksum */
    char    volume_name[16];    /* Volume name (zero-terminated) */
} romdisk_hdr_t;

/* File header info; note that this header plus filename must be a multiple of
   16 bytes, and the following file data must also be a multiple of 16 bytes. */
typedef struct {
    uint32  next_header;        /* Offset of next header */
    uint32  spec_info;      /* Spec info */
    uint32  size;           /* Data size */
    uint32  checksum;       /* File checksum */
    char    filename[16];       /* File name (zero-terminated) */
} romdisk_file_t;


/* Util function to reverse the byte order of a uint32 */
static uint32 ntohl_32(const void *data) {
    const uint8 *d = (const uint8*)data;
    return (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | (d[3] << 0);
}

/* Pointer to the ROMFS image created by the host PC */
static uint8 *romdisk_image = NULL;

/* Some convenience pointers */
static romdisk_hdr_t *romdisk_hdr = NULL;
static uint32 romdisk_files = 0;

/********************************************************************************/
/* File primitives */

/* File handles.. I could probably do this with a linked list, but I'm just
   too lazy right now. =) */
static struct {
    uint32      index;      /* romfs image index */
    int     dir;        /* >0 if a directory */
    uint32      ptr;        /* Current read position in bytes */
    uint32      size;       /* Length of file in bytes */
    /* dirent_t dirent; */  /* A static dirent to pass back to clients */
} fh[MAX_RD_FILES];

/* Mutex for file handles */
static thd_mutex_t fh_mutex;

/* Given a filename and a starting romdisk directory listing (byte offset),
   search for the entry in the directory and return the byte offset to its
   entry. */
static uint32 romdisk_find_object(const char *fn, int fnlen, int dir, uint32 offset) {
    uint32      i, ni, type;
    romdisk_file_t  *fhdr;

    i = offset;

    do {
        /* Locate the entry, next pointer, and type info */
        fhdr = (romdisk_file_t *)(romdisk_image + i);
        ni = ntohl_32(&fhdr->next_header);
        type = ni & 0x0f;
        ni = ni & 0xfffffff0;

        /* Check the type */
        if(!dir) {
            if((type & 3) != 2) {
                i = ni;

                if(!i)
                    break;
                else
                    continue;
            }
        }
        else {
            if((type & 3) != 1) {
                i = ni;

                if(!i)
                    break;
                else
                    continue;
            }
        }

        /* Check filename */
        if(!strncmp(fhdr->filename, fn, fnlen)) {
            /* Match: return this index */
            return i;
        }

        i = ni;
    }
    while(i != 0);

    /* Didn't find it */
    return 0;
}

/* Locate an object anywhere in the image, starting at the root, and
   expecting a fully qualified path name. This is analogous to the
   find_object_path in iso9660.

   fn:      object filename (absolute path)
   dir:     0 if looking for a file, 1 if looking for a dir

   It will return an offset in the romdisk image for the object. */
static uint32 romdisk_find(const char *fn, int dir) {
    const char  *cur;
    uint32      i;
    romdisk_file_t  *fhdr;

    /* If the object is in a sub-tree, traverse the trees looking
       for the right directory. */
    i = romdisk_files;

    while((cur = strchr(fn, '/'))) {
        if(cur != fn) {
            i = romdisk_find_object(fn, cur - fn, 1, i);

            if(i == 0) return 0;

            fhdr = (romdisk_file_t *)(romdisk_image + i);
            i = ntohl_32(&fhdr->spec_info);
        }

        fn = cur + 1;
    }

    /* Locate the file in the resulting directory */
    if(*fn) {
        i = romdisk_find_object(fn, strlen(fn), dir, i);
        return i;
    }
    else {
        if(!dir)
            return 0;
        else
            return i;
    }
}


#if 0
/* Find a filename in the root of the romdisk image */
static uint32 romdisk_find(const char *fn) {
    uint32      i, ni, type;
    romdisk_file_t  *fhdr;

    i = romdisk_files;

    do {
        /* Locate the file, next pointer, and type info */
        fhdr = (romdisk_file_t*)(romdisk_image + i);
        ni = ntohl_32(&fhdr->next_header);
        type = ni & 0x0f;
        ni = ni & 0xfffffff0;

        /* Ignore non-files */
        if(type != 2) {
            i = ni;
            continue;
        }

        /* Check filename */
        if(!strcmp(fhdr->filename, fn)) {
            /* Match: return this index */
            return i;
        }

        /* Nope, continue on */
        i = ni;
    }
    while(i != 0);

    /* Didn't find it */
    return 0;
}
#endif

/* Open a file or directory */
static uint32 romdisk_open(const char *fn, int mode) {
    uint32      fd;
    uint32      filehdr;
    romdisk_file_t  *fhdr;

    /* Make sure they don't want to open things as writeable */
    if((mode & O_MODE_MASK) != O_RDONLY)
        return 0;

    /* Make sure we're not trying to open a directory (not supported yet) */
    if(mode & O_DIR)
        return 0;

    /* Look for the file */
    filehdr = romdisk_find(fn + 1, 0);

    if(filehdr == 0)
        return 0;

    /* Find a free file handle */
    thd_mutex_lock(&fh_mutex);

    for(fd = 0; fd < MAX_RD_FILES; fd++)
        if(fh[fd].index == 0) {
            fh[fd].index = -1;
            break;
        }

    thd_mutex_unlock(&fh_mutex);

    if(fd >= MAX_RD_FILES)
        return 0;

    /* Fill the fd structure */
    fhdr = (romdisk_file_t *)(romdisk_image + filehdr);
    fh[fd].index = filehdr + sizeof(romdisk_file_t) + (strlen(fhdr->filename) / 16) * 16;
    fh[fd].dir = 0;
    fh[fd].ptr = 0;
    fh[fd].size = ntohl_32(&fhdr->size);

    return fd;
}

/* Close a file or directory */
static void romdisk_close(uint32 fd) {
    /* Check that the fd is valid */
    if(fd < MAX_RD_FILES) {
        /* No need to lock the mutex: this is an atomic op */
        fh[fd].index = 0;
    }
}

/* Read from a file */
static ssize_t romdisk_read(uint32 fd, void *buf, size_t bytes) {
    /* Check that the fd is valid */
    if(fd >= MAX_RD_FILES || fh[fd].index == 0)
        return -1;

    /* Is there enough left? */
    if((fh[fd].ptr + bytes) > fh[fd].size)
        bytes = fh[fd].size - fh[fd].ptr;

    /* Copy out the requested amount */
    memcpy(buf, romdisk_image + fh[fd].index + fh[fd].ptr, bytes);
    fh[fd].ptr += bytes;

    return bytes;
}

/* Seek elsewhere in a file */
static off_t romdisk_seek(uint32 fd, off_t offset, int whence) {
    /* Check that the fd is valid */
    if(fd >= MAX_RD_FILES || fh[fd].index == 0)
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
static off_t romdisk_tell(uint32 fd) {
    if(fd >= MAX_RD_FILES || fh[fd].index == 0)
        return -1;

    return fh[fd].ptr;
}

/* Tell how big the file is */
static size_t romdisk_total(uint32 fd) {
    if(fd >= MAX_RD_FILES || fh[fd].index == 0)
        return -1;

    return fh[fd].size;
}

/* Read a directory entry */
dirent_t *romdisk_readdir(uint32 fd) {
    /*  int     i, c;

        if (fd>=MAX_RD_FILES || fh[fd].index==0 || !fh[fd].dir)
            return NULL;

        return &fh[fd].dirent; */
    return NULL;
}

/* Put everything together */
static vfs_handler vh = {
    0, 0,       /* In-kernel, no caching */
    romdisk_open,
    romdisk_close,
    romdisk_read,
    NULL,
    romdisk_seek,
    romdisk_tell,
    romdisk_total,
    romdisk_readdir,
    NULL,
    NULL
};

/* Initialize the file system */
static int fs_romdisk_init(uint8 *img) {
    int i, ni;
    romdisk_file_t *fhdr;

    /* Set the ROMFS image */
    romdisk_image = img;

    /* Check and print some info about it */
    romdisk_hdr = (romdisk_hdr_t *)romdisk_image;

    if(strncmp((char*)romdisk_image, "-rom1fs-", 8)) {
        printf("Rom disk image at 0x%p is not a ROMFS image\r\n", img);
        return -1;
    }

    printf("ROMFS image recognized. Full size is 0x%lx bytes\r\n",
           ntohl_32(&romdisk_hdr->full_size));
    printf("  Checksum is 0x%lx\r\n",
           ntohl_32(&romdisk_hdr->checksum));
    printf("  Volume ID is ``%s''\r\n",
           romdisk_hdr->volume_name);

    romdisk_files = sizeof(romdisk_hdr_t)
                    + (strlen(romdisk_hdr->volume_name) / 16) * 16;
    printf("  File entries begin at offset 0x%lx\r\n",
           romdisk_files);

    printf("Files:\r\n");
    i = romdisk_files;

    do {
        printf("0x%x: ", i);

        fhdr = (romdisk_file_t*)(romdisk_image + i);
        ni = ntohl_32(&fhdr->next_header);
        printf("next=%x, ", ni & 0xfffffff0);
        printf("type=%x, ", ni & 0x0f);
        ni &= 0xfffffff0;
        printf("spec_info=%lx, ", ntohl_32(&fhdr->spec_info));
        printf("size=%lx, ", ntohl_32(&fhdr->size));
        printf("checksum=%lx ", ntohl_32(&fhdr->checksum));
        printf("filename='%s'\r\n", fhdr->filename);
        printf("  File data starts at %lx\r\n",
               i + sizeof(romdisk_file_t)
               + (strlen(fhdr->filename) / 16) * 16);

        i = ni;
    }
    while(i != 0);

    /* Reset fd's */
    memset(fh, 0, sizeof(fh));

    /* Mark the first as active so we can have an error FD of zero */
    fh[0].index = -1;

    /* Init thread mutexes */
    thd_mutex_reset(&fh_mutex);

    /* Register with VFS */
    return fs_handler_add("/rd", &vh);
}

/* De-init the file system */
static int fs_romdisk_shutdown() {
    return fs_handler_remove(&vh);
}



/* Cut here to insert into KallistiOS fs_iso9660.c */

/********************************************************************************/

/**
 * Helper function to load the raw texture data into an array.
 * @param filename The filename of the texture.
 * @param data A pointer to an array where the data should be stored.
 * @param size A pointer to a variable where to size should be stored.
 * @return 0 on success, non-zero on error.
 */
static int read_file_contents(char const * const filename, char **data, size_t *size) {
    FILE *f = 0;

    f = fopen(filename, "rb");

    if(!f) {
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long imageSize = ftell(f);
    rewind(f);

    *data = malloc(imageSize);
    if(!*data) {
        fclose(f);
        return 2;
    }

    if(fread(*data, imageSize, 1, f) != 1) {
        free(*data);
        fclose(f);
        return 3;
    }

    fclose(f);

    *size = imageSize;

    return 0;
}

static int init() {
    uint8 *img;
    size_t  size;

    if(read_file_contents("romdisk2.img", (char**)&img, &size)) {
        fprintf(stderr, "Cannot read romdisk2.img.\n");
        return 1;
    }

    fs_romdisk_init(img);
    fs_romdisk_shutdown();

    free(img);

    return 0;
}

int main() {
    if(init()) {
        fprintf(stderr, "Cannot init.\n");
        return 1;
    }

    {
        uint32  fd, size;
        char    buf[667];

        printf("Opening file /testdir/rdtest.c\n");
        fd = romdisk_open("/testdir/rdtest.c", O_RDONLY);

        if(fd == 0) {
            printf("Couldn't open file\n");
            return 1;
        }

        size = romdisk_total(fd);
        printf("fd is %ld, size is %08lx\n", fd, size);

        while(size > 0) {
            int r;
            r = romdisk_read(fd, buf, 666);

            if(r < 0) {
                printf("Read error\n");
                return 1;
            }

            buf[r] = 0;
            printf("%s", buf);
            size -= r;
        }

        printf("\nFile read is done\n");
        romdisk_close(fd);
    }

    /* {
        uint32      fd, t;
        dirent_t    *de;

        printf("Opening /demos");
        fd = iso_open("/music/mc2/entries/rookie", O_RDONLY | O_DIR);
        if (fd == 0) {
            printf("Couldn't open file\n");
            return;
        }

        printf("Scanning dir:\n");
        while ( (de = iso_readdir(fd)) ) {
            printf("%s\t%d\n", de->name, de->size);
            if (!strcmp(de->name, "R_WAR.ZIP")) {
                t = 0;
            }
        }

        iso_close(fd);
    } */

    return 0;
}
