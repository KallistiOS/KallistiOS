/* KallistiOS ##version##

   kmgenc.c
   Copyright (C)2003 Megan Potter
   Portions Copyright (C)2002 Gil Megidish

   This utility loads JPGs and PNGs and converts them into KMGs of the
   specified format. Right now only Dreamcast formats are supported.
   KMGs are basically just raw texture data that can be DMA'd straight
   to the PVR's memory. This is useful to improve loading times, among
   other things.

   The following options are available:
   - Twiddling (or no)
   - Mipmaps (or no)
   - VQ encoding (or no)

   Any combination of these attributes may be selected for the final
   output file. Note that input textures must be a power of 2 on each
   side, though they may not necessarily need to be square. VQ'd textures
   need to be square (supposedly).

   The following output formats are available as well:
   - RGB565
   - ARGB4444
   - ARGB1555

   The ARGB formats are not particularly useful unless you're using
   a source PNG with an alpha channel. If you select an RGB format with
   such a file, the alpha channel will be silently ignored (you may get
   background noise depending on your PNG creation program).


   XXX
   Note: this is pretty darned unfinished and needs to be combined with
   vqenc to be complete. For now (since vqenc outputs KMGs as well) I've
   just put in the basic twiddling operations.

*/

#include "kmgenc.h"

int use_twiddle = 1;
int use_verbose = 1;
int use_debug = 1;
int use_alpha = 0;

/* Linear/iterative twiddling algorithm from Marcus' tatest */
#define TWIDTAB(x) ( (x&1)|((x&2)<<1)|((x&4)<<2)|((x&8)<<3)|((x&16)<<4)| \
                     ((x&32)<<5)|((x&64)<<6)|((x&128)<<7)|((x&256)<<8)|((x&512)<<9) )
#define TWIDOUT(x, y) ( TWIDTAB((y)) | (TWIDTAB((x)) << 1) )
#define MIN(a, b) ( (a)<(b)? (a):(b) )

/* This twiddling code is copied from pvr_texture.c, and the original
   algorithm was written by Vincent Penne. */
static void twiddle(image_t * src, uint16 * output) {
    int w = src->w;
    int h = src->h;
    int min = MIN(w, h);
    int mask = min - 1;
    uint16 * pixels = (uint16 *)src->data;
    uint16 * vtex = output;
    int x, y, yout;

    for(y = 0; y < h; y++) {
        /* yout = ((h - 1) - y); */
        yout = y;

        for(x = 0; x < w; x++) {
            vtex[TWIDOUT(x & mask, yout & mask) +
                 (x / min + yout / min)*min * min] = pixels[y * w + x];
        }
    }
}

static void convert_to_16(image_t * img) {
    int i;
    fcolor_t fc;
    uint16 * out;

    out = malloc(img->w * img->h * 2);

    for(i = 0; i < img->w * img->h; i++) {
        get_fcolor_32(&fc, img->data + i * img->bpp);
        LIMITFC(fc);

        switch(use_alpha) {
            case 0:
                out[i] = le16(PACK565(fc.r, fc.g, fc.b));
                break;
            case 1:
                out[i] = le16(PACK4444(fc.a, fc.r, fc.g, fc.b));
                break;
            case 2:
                out[i] = le16(PACK1555(fc.a, fc.r, fc.g, fc.b));
                break;
        }
    }

    img->bpp = 2;
    free(img->data);
    img->data = (uint8 *)out;
}

static int save(const char *filename, image_t *img) {
    FILE    *fp;
    kmg_header_t    hdr;
    uint16      * tmp = NULL;
    int     fmt, cnt;

    fp = fopen(filename, "wb");

    if(fp == NULL) {
        fprintf(stderr, "FATAL: cannot create %s\n", filename);
        return -errno;
    }

    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = le32(KMG_MAGIC);
    hdr.version = le32(KMG_VERSION);
    hdr.platform = le32(KMG_PLAT_DC);

    switch(use_alpha) {
        case 0:
            fmt = KMG_DCFMT_RGB565;
            break;
        case 1:
            fmt = KMG_DCFMT_ARGB4444;
            break;
        case 2:
            fmt = KMG_DCFMT_ARGB1555;
            break;
        default:
            exit(-1);
    }

    if(use_twiddle)
        fmt |= KMG_DCFMT_TWIDDLED;

    hdr.format = le32(fmt);
    hdr.width = le32(img->w);
    hdr.height = le32(img->h);
    cnt = img->w * img->h * 2;
    hdr.byte_count = le32(cnt);

    if(fwrite(&hdr, sizeof(hdr), 1, fp) != 1) {
        fprintf(stderr, "FATAL: can't write KMG header to %s\n", filename);
        goto loser;
    }

    /* Twiddle the image into a temp buffer */
    tmp = malloc(cnt);
    twiddle(img, tmp);

    /* Write it out */
    if(fwrite(tmp, cnt, 1, fp) != 1) {
        fprintf(stderr, "FATAL: can't write KMG data to %s\n", filename);
        goto loser;
    }

    free(tmp);
    fclose(fp);
    return 0;

loser:

    if(tmp)
        free(tmp);

    fclose(fp);
    unlink(filename);
    return -1;
}


static void banner(const char *progname) {
    printf("Usage: %s [options] image1 [image2..]\n", progname);
    printf("\n");
    printf("Options:\n");
    /* printf("\t-t, --twiddle\tcreate twiddled textures\n"); */
    /* printf("\t-m, --mipmap\tcreate mipmapped textures (EXPERIMENTAL)\n"); */
    printf("\t-v, --verbose\tverbose\n");
    printf("\t-d, --debug\tshow debug information\n");
    /* printf("\t-q, --highq\thigher quality (much slower)\n"); */
    printf("\t-a4, --argb4444\tuse alpha channel (and output ARGB4444)\n");
    printf("\t-a1, --argb1555\tuse alpha channel (and output ARGB1555)\n");
}

static int valid_size(int x) {
    if(x < 2 || x > 1024)
        return 0;

    while(x) {
        int bit = (x & 1);
        x >>= 1;

        if(x && bit) {
            /* more than one bit in image */
            return 0;
        }
    }

    return 1;
}

static const char *figure_outfilename(const char *f, const char *newext) {
    char *newname;
    char *ext;

    ext = strrchr(f, '.');

    if(ext == NULL) {
        newname = (char *)malloc(strlen(f) + strlen(newext) + 2);

        if(newname) {
            sprintf(newname, "%s.%s", f, newext);
        }
    }
    else {
        int len;

        len = (ext - f) + strlen(newext) + 2;
        newname = (char *)malloc(len);

        if(newname) {
            strcpy(newname, f);
            ext = strrchr(newname, '.') + 1;
            strcpy(ext, newext);
        }
    }

    return newname;
}

static void destroy_image(image_t *image) {
    if(image->data) {
        free(image->data);
        image->data = NULL;
    }
}

static int encode(const char *infile) {
    int     ok;
    image_t     image;
    const char  *outfile;

    if(use_verbose) {
        printf("encoding %s.. ", infile);
    }

    if(get_image(infile, &image) < 0) {
        fprintf(stderr, "failed reading %s\n", infile);
        return -EINVAL;
    }

    if(valid_size(image.w) == 0 || valid_size(image.h) == 0) {
        fprintf(stderr, "image dimensions for %s are not valid, see manual\n", infile);
        destroy_image(&image);
        return -EINVAL;
    }

    outfile = figure_outfilename(infile, "kmg");

    if(outfile == NULL) {
        fprintf(stderr, "memory allocation failed for %s\n", infile);
        return -ENOMEM;
    }

    /* Convert the input image to a 16-bit image according to parameters */
    convert_to_16(&image);

    /* Save it */
    ok = save(outfile, &image);

    destroy_image(&image);

    printf("\n");
    return ok;
}

static int process_long_options(char *arg) {
    /* if (! strcmp(arg, "mipmap"))
        use_mipmap = 1;
    else if (! strcmp(arg, "twiddle"))
        use_twiddle = 1;
    else */ if(! strcmp(arg, "debug"))
        use_debug = 1;
    else if(! strcmp(arg, "verbose"))
        use_verbose = 1;
    /* else if (! strcmp(arg, "highq"))
        use_hq = 1; */
    else if(! strcmp(arg, "alpha"))
        use_alpha = 1;
    else
        return -EINVAL;

    return 0;
}

static int process_option(char *arg) {
    /* assuming starts with '-' */
    arg++;

    switch(*arg) {

            /* case 'm':
                use_mipmap = 1;
                return 0; */

            /* case 't':
                use_twiddle = 1;
                return 0; */

        case 'v':
            use_verbose = 1;
            return 0;

        case 'd':
            use_debug = 1;
            return 0;

            /* case 'q':
                use_hq = 1;
                return 0; */

        case 'a':
            arg++;

            if(*arg == '4')
                use_alpha = 1;
            else if(*arg == '1')
                use_alpha = 2;
            else
                return -EINVAL;

            return 0;

        case '-':
            return process_long_options(arg + 1);
    }

    return -EINVAL;
}

static int process(int argc, char *argv[]) {
    int arg;

    arg = 1;

    while(arg < argc) {
        if(argv[arg][0] == '-') {
            if(process_option(argv[arg]) < 0) {
                fprintf(stderr, "invalid option %s\n", argv[arg]);
                return -EINVAL;
            }

            arg++;
            continue;
        }

        /* found a filename */
        break;
    }

    if(arg >= argc) {
        fprintf(stderr, "no files to encode\n");
        return -EINVAL;
    }

    while(arg < argc) {
        /* ordinary image */
        encode(argv[arg]);
        arg++;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    setbuf(stdout, 0);

    if(argc < 2) {
        banner(argv[0]);
        return 0;
    }

    return process(argc, argv);
}
