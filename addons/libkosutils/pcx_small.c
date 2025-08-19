/* KallistiOS ##version##

   pcx_small.c
   (c)2000-2001 Megan Potter

   PCX image loader
*/

#include <stdio.h>
#include <kos/fs.h>
#include <kos/pcx.h>

/* PCX header structure */
typedef struct {
    char   mfg;               /* manufacturer, always 0xa0      */
    char   ver;               /* encoder version number (5)     */
    char   enc;               /* encoding code, always 1        */
    char   bpp;               /* bits per pixel, 8 in mode 0x13 */
    uint16_t xmin, ymin;      /* image origin, usually 0,0      */
    uint16_t xmax, ymax;      /* image dimensions           */
    uint16_t hres;            /* horizontal resolution value    */
    uint16_t vres;            /* vertical resolution value      */
    char   pal[48];           /* palette (not in mode 0x13)     */
    char   reserved;          /* who knows?             */
    char   clrplanes;         /* number of planes, 1 in mode 0x13   */
    uint16_t bpl;             /* bytes per line, 80 in mode 0x13    */
    uint16_t pltype;          /* Grey or Color palette flag     */
    char   filler[58];        /* Zsoft wanted a 128 byte header */
} __packed pcx_hdr;

/* This version converts directly into 16-bit BGR data */
int pcx_load_flat(const char *fn, int *w_out, int *h_out, void *pic_out) {
    pcx_hdr *pcxh;
    file_t  fd;
    uint8_t *pcx, *pal;
    int bytes = 0;  /* counts unpacked bytes */
    uint8_t     c;  /* byte being processed */
    uint16_t    oc; /* output pixel */
    int runlen;     /* length of packet */
    int num_bytes;
    uint16_t    *iout;

    /* Open the file */
    fd = fs_open(fn, O_RDONLY);

    if(fd == 0) {
        printf("pcx_load(%s): Couldn't open file\n", fn);
        return -1;
    }

    pcx = fs_mmap(fd);

    if(pcx == NULL) {
        printf("pcx_load(%s): Couldn't mmap file\n", fn);
        fs_close(fd);
        return -2;
    }

    /* Load the PCX header */
    pcxh = (pcx_hdr *)(pcx + 0);

    if(pcxh->bpp != 8) {
        printf("pcx_load(%s): PCX data is not 8bpp\n", fn);
        fs_close(fd);
        return -3;
    }

    /* Locate the palette (hopefully ~=) */
    pal = pcx + (fs_total(fd) - 768);

    /* Setup output parameters */
    *w_out = pcxh->xmax + 1;
    *h_out = pcxh->ymax + 1;
    num_bytes = *w_out * *h_out;

    /* Skip header */
    pcx += sizeof(pcx_hdr);

    /* uint16_t version of the output buffer */
    iout = (uint16_t *)pic_out;

    do {
        c = *pcx++;     /* Read one byte */

        if((c & 0xc0) == 0xc0) {    /* high 2 bits set is packet */
            runlen = (c & 0x3f);    /* AND off the high bits */
            c = *pcx++;

            /* Decode the pixel data (BGR555) */
            oc =
                ((pal[c * 3 + 0] >> 3) << 0) |
                ((pal[c * 3 + 1] >> 3) << 5) |
                ((pal[c * 3 + 2] >> 3) << 10);

            while(runlen--)
                iout[bytes++] = oc;
        }
        else {
            /* Decode the pixel data (BGR555) */
            oc =
                ((pal[c * 3 + 0] >> 3) << 0) |
                ((pal[c * 3 + 1] >> 3) << 5) |
                ((pal[c * 3 + 2] >> 3) << 10);
            iout[bytes++] = oc;
        }
    }
    while(bytes < num_bytes);

    /* Close the file */
    fs_close(fd);

    return 0;
}

/* This version writes out a separate 15-bit BGR palette and decodes the paletted data
   into another buffer 16 bits at a time. */
int pcx_load_palette(const char *fn, int *w_out, int *h_out, void *pic_out, void *pal_out) {
    pcx_hdr *pcxh;
    file_t  fd;
    uint8_t   *pcx, *pal;
    int bytes = 0;      /* counts unpacked bytes */
    uint8_t   c;        /* byte being processed */
    uint16_t  oc = 0;   /* output pixel */
    int runlen, i;      /* length of packet */
    int num_bytes;
    uint16_t  *iout;

    /* Open the file */
    fd = fs_open(fn, O_RDONLY);

    if(fd == 0) {
        printf("pcx_load(%s): Couldn't open file\n", fn);
        return -1;
    }

    pcx = fs_mmap(fd);

    if(pcx == NULL) {
        printf("pcx_load(%s): Couldn't mmap file\n", fn);
        fs_close(fd);
        return -2;
    }

    /* Load the PCX header */
    pcxh = (pcx_hdr *)(pcx + 0);

    if(pcxh->bpp != 8) {
        printf("pcx_load(%s): PCX data is not 8bpp\n", fn);
        fs_close(fd);
        return -3;
    }

    /* Locate the palette (hopefully ~=) */
    pal = pcx + (fs_total(fd) - 768);

    /* Write the palette into the output buffer */
    iout = (uint16_t *)pal_out;

    for(i = 0; i < 256; i++) {
        iout[i] =
            ((pal[0] >> 3) << 0) |
            ((pal[1] >> 3) << 5) |
            ((pal[2] >> 3) << 10);
        pal += 3;
    }

    /* Setup output parameters */
    *w_out = pcxh->xmax + 1;
    *h_out = pcxh->ymax + 1;
    num_bytes = *w_out * *h_out;

    /* Skip header */
    pcx += sizeof(pcx_hdr);

    /* uint16_t version of the output buffer */
    iout = (uint16_t *)pic_out;
    i = 0;

    do {
        c = *pcx++;     /* Read one byte */

        if((c & 0xc0) == 0xc0) {    /* high 2 bits set is packet */
            runlen = (c & 0x3f);    /* AND off the high bits */
            c = *pcx++;

            while(runlen--) {
                if(!i) {
                    oc = c;
                    bytes++;
                    i = 1;
                }
                else {
                    iout[(bytes >> 1)] = oc | (c << 8);
                    bytes++;
                    i = 0;
                }
            }
        }
        else {
            if(!i) {
                oc = c;
                bytes++;
                i = 1;
            }
            else {
                iout[(bytes >> 1)] = oc | (c << 8);
                bytes++;
                i = 0;
            }
        }
    }
    while(bytes < num_bytes);

    /* Close the file */
    fs_close(fd);

    return 0;
}


