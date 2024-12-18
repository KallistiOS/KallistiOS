/* KallistiOS ##version##

   vqenc.c
   Copyright (C)2002 Gil Megidish

   Compresses JPG images into PowerVR supported VQ textures. All images
   must be squared. Twiddled and mipmapped toggles are supported. I feel
   dizzy, I think I over-twiddled.

   This code is based on the work of Jonas Norberg, you can find more info at
   http://www.acc.umu.se/~bedev/software/vq/
*/

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include "get_image.h"
#include "vq_internal.h"
#include "vq_types.h"

/* For outputting KMG files */
#include "kmg.h"

static int use_mipmap = 0;
static int use_twiddle = 0;
static int use_verbose = 0;
static int use_debug = 0;
static int use_hq = 0;
static int use_kmg = 0;
static int use_alpha = 0;

#define PACK1555(a, r, g, b) ( (a ? 0x8000 : 0) | ((r>>3)<<10) | ((g>>3)<<5) | ((b >>3)))
#define PACK4444(a, r, g, b) ( ((a>>4) << 12) | ((r>>4)<<8) | ((g>>4)<<4) | ((b>>4)) )
#define PACK565(r, g, b) (((r>>3)<<11) | ((g>>2)<<5) | ((b>>3)))
#define LIMIT(x,low,high) (x) < low ? low : (x) > high ? high : (x);

static void reset_code(code_t *c) {
    clear_quad(&c->pos_sum);
    c->pos_count = 0;
    c->max_dist = 0.0;
}

static void reset_codebook(context_t *cb) {
    int i;
    code_t *e;

    e = cb->codes;

    for(i = 0; i < cb->in_use; i++) {
        reset_code(e);
        e++;
    }
}


static double quad_length(fquad_t *q) {
    int i;
    float total;

    total = 0.0;

    for(i = 0; i < 4; i++) {
        total += (q->p[i].a * q->p[i].a);
        total += (q->p[i].r * q->p[i].r);
        total += (q->p[i].g * q->p[i].g);
        total += (q->p[i].b * q->p[i].b);
    }

    return sqrt(total);
}

static int map_width(int res) {
    int width;

    width = (1 << (res + 1));
    return width;
}

static int quads_in_map(int res) {
    int across;

    across = (1 << (res + 1));
    return (across * across) >> 2;
}

static double delta_e(fquad_t *a, fquad_t *b) {
    int i;
    fquad_t sub;
    double total;

    total = 0.0;
    sub_quad(&sub, a, b);

    for(i = 0; i < 4; i++) {
        total += (sub.p[i].a * sub.p[i].a);
        total += (sub.p[i].r * sub.p[i].r);
        total += (sub.p[i].g * sub.p[i].g);
        total += (sub.p[i].b * sub.p[i].b);
    }

    return sqrt(total);
}

/* returns the closest (most similar) codebook entry to the given quad */
static int find(context_t *cb, fquad_t *q) {
    int code, close_entry;
    double close_dist;

    close_entry = 0;
    close_dist = delta_e(&cb->codes[0].value, q);

    for(code = 1; code < cb->in_use; code++) {

        /* hope not to get sued for this variable's name */
        double d;

        d = delta_e(&cb->codes[code].value, q);

        if(d < close_dist) {
            close_entry = code;
            close_dist = d;

            if(d < 0.0001) {
                /* close enough */
                return close_entry;
            }
        }

    }

    return close_entry;
}

static void place(context_t *cb, fquad_t *quads, int nquads) {
    int i, idx;
    code_t *e;
    double dist;
    fquad_t *that;

    that = quads;

    for(i = 0; i < nquads; i++) {
        /* find averages of all codebook entries */
        idx = find(cb, that);
        e = &cb->codes[idx];

        add_quad(&e->pos_sum, that);
        e->pos_count++;

        /* see if we have something better in hand */
        dist = delta_e(&e->value, that);

        if(dist > e->max_dist) {
            e->max_dist = dist;
            copy_quad(&e->max_dist_vec, that);
        }

        that++;
    }
}

static void clean_codebook(context_t *cb) {
    int i;
    code_t * e;

    e = cb->codes;

    for(i = 0; i < cb->in_use; i++) {
        if(e->pos_count > 0) {
            /* code has been used */
            div_quad(&e->pos_sum, (float)e->pos_count);
            copy_quad(&e->value, &e->pos_sum);
            e++;
        }
        else {
            /* never been used */
            *e = cb->codes[cb->in_use - 1];
            cb->in_use--;
            e++;
        }
    }
}

/* add new quad to codebook */
static int add_to_codebook(context_t *cb, fquad_t *q) {
    int index;

    index = cb->in_use;
    reset_code(&cb->codes[index]);
    copy_quad(&cb->codes[index].value, q);

    cb->codes[index].index = index;

    /* update sequential id */
    cb->in_use++;
    return index;
}

/* based on the statistics, split the entries in the codebook table */
static void split(context_t *cb) {
    int     i, elements;
    code_t  *e;
    fquad_t new_element;

    e = cb->codes;
    elements = cb->in_use;

    for(i = 0; i < elements; i++) {
        if(e->pos_count > 1) {

            fquad_t diff;
            float len;

            sub_quad(&diff, &e->max_dist_vec, &e->value);
            len = quad_length(&diff) * 256.0f;
            div_quad(&diff, len);

            copy_quad(&new_element, &e->value);
            add_quad(&new_element, &diff);

            sub_quad(&e->value, &e->value, &diff);
        }
        else {
            /* some elements were not used, so we'll
             * merge those with a black (zero) quad and hope
             * some other quad will find it useful.
             */
            copy_quad(&new_element, &e->value);
            div_quad(&new_element, 2.0);
        }

        add_to_codebook(cb, &new_element);
        /* fixme: remove those that are <= 0 */
        e++;
    }
}

static int new_context(context_t *cb) {
    reset_code(&cb->codes[0]);
    clear_quad(&cb->codes[0].value);
    cb->codes[0].index = 0;

    /* only one color supported, and it's black */
    cb->in_use = 1;
    return 0;
}

static uint16 pack(fcolor_t *c) {
    int a, r, g, b;

    a = LIMIT(c->a, 0, 255);
    r = LIMIT(c->r, 0, 255);
    g = LIMIT(c->g, 0, 255);
    b = LIMIT(c->b, 0, 255);

    if(use_alpha == 1)
        return PACK4444(a, r, g, b);
    else if(use_alpha == 2)
        return PACK1555(a, r, g, b);
    else
        return PACK565(r, g, b);
}

static int inline le16(int x) {
    /* Endian test added by Megan. This is probably not too efficient
       but it's portable and will get the job done. */
    unsigned long test = 0x12345678;
    unsigned char * tp = (unsigned char *)&test;

    if(*tp == 0x78)
        /* Little endian */
        return x;
    else
        /* Big endian */
        return ((x << 8) & 0xff00) | ((x >> 8) & 0x00ff);
}

static void copy_codebook(context_t *cb, uint16 *codebook) {
    int     i;
    code_t  *e;

    e = cb->codes;

    for(i = 0; i < cb->in_use; i++) {
        /* even the codebook is twiddled! */
        *codebook++ = le16(pack(&e->value.p[0]));
        *codebook++ = le16(pack(&e->value.p[2]));
        *codebook++ = le16(pack(&e->value.p[1]));
        *codebook++ = le16(pack(&e->value.p[3]));
        e++;
    }

    /* fill up what's left */
    while(i < 256) {
        *codebook++ = 0;
        *codebook++ = 0;
        *codebook++ = 0;
        *codebook++ = 0;
        i++;
    }
}

static int divide(int *ptr, int stride, int x, int y, int blocksize, int seq) {
    int before;

    before = seq;

    switch(blocksize) {
        case 1:
            /* can't divide anymore */
            ptr[seq++] = y * stride + x;
            break;

        default:
            blocksize = blocksize >> 1;
            seq += divide(ptr, stride, x, y, blocksize, seq);
            seq += divide(ptr, stride, x, y + blocksize, blocksize, seq);
            seq += divide(ptr, stride, x + blocksize, y, blocksize, seq);
            seq += divide(ptr, stride, x + blocksize, y + blocksize, blocksize, seq);
            break;
    }

    return (seq - before);
}

static int *twiddle_twiddle(int length) {
    /* divide and conquer */
    int *ptr = (int *)malloc(sizeof(int) * length * length);

    if(ptr == NULL)
        return NULL;

    divide(ptr, length, 0, 0, length, 0);
    return ptr;
}

static int write_linear(FILE *out, context_t *cb, mipmap_t *m, int res) {
    int i, nquads;

    nquads = quads_in_map(res);

    for(i = 0; i < nquads; i++) {
        uint8 c = find(cb, &m->map[res][i]);

        if(fputc(c, out) == EOF)
            return -1;
    }

    return 0;
}

static int write_twiddled(FILE *out, context_t *cb, mipmap_t *m, int res) {
    int *twididx, *twiddled;
    int i, width, nquads;
    fquad_t *map;

    width = map_width(res);
    nquads = quads_in_map(res);

    twiddled = twiddle_twiddle(width / 2);
    twididx = twiddled;
    map = m->map[res];

    for(i = 0; i < nquads; i++) {
        uint8 c = find(cb, &map[*twididx++]);

        if(fputc(c, out) == EOF)
            return -1;
    }

    free(twiddled);
    return 0;
}

static int save_codebook(FILE *out, context_t *cb) {
    uint16 codebook[256 * 4];

    copy_codebook(cb, codebook);

    if(fwrite(codebook, sizeof(codebook), 1, out) != 1) {
        /* bummer */
        return -errno;
    }

    return 0;
}

static int save(const char *filename, context_t *cb, mipmap_t *m, image_t *img) {
    int ok, res;
    FILE    *fp;

    fp = fopen(filename, "wb");

    if(fp == NULL) {
        fprintf(stderr, "FATAL: cannot create %s\n", filename);
        return -errno;
    }

    if(use_kmg) {
        kmg_header_t    hdr;

        memset(&hdr, 0, sizeof(hdr));
        hdr.magic = KMG_MAGIC;
        hdr.version = KMG_VERSION;
        hdr.platform = KMG_PLAT_DC;

        if(use_alpha == 1)
            hdr.format = KMG_DCFMT_ARGB4444 | KMG_DCFMT_VQ;
        else if(use_alpha == 2)
            hdr.format = KMG_DCFMT_ARGB1555 | KMG_DCFMT_VQ;
        else
            hdr.format = KMG_DCFMT_RGB565 | KMG_DCFMT_VQ;

        if(use_twiddle)
            hdr.format |= KMG_DCFMT_TWIDDLED;

        if(use_mipmap)
            hdr.format |= KMG_DCFMT_MIPMAP;

        hdr.width = img->w;
        hdr.height = img->h;
        hdr.byte_count = 2048 + (use_mipmap ? 1 : 0);

        for(res = 0; res < MAX_MIPMAP; res++) {
            if(m->map[res] != NULL) {
                hdr.byte_count += quads_in_map(res);
            }
        }

        if(fwrite(&hdr, sizeof(hdr), 1, fp) != 1) {
            fprintf(stderr, "FATAL: can't write KMG header to %s\n", filename);
            goto loser;
        }
    }

    if(save_codebook(fp, cb) < 0) {
        fprintf(stderr, "FATAL: failed writing codebook to %s\n", filename);
        goto loser;
    }

    /* dummy byte (0) must be included in square mipmaps */
    if(use_mipmap) {
        if(fputc('\0', fp) == EOF) {
            fprintf(stderr, "FATAL: error writing %s\n", filename);
            goto loser;
        }
    }

    for(res = 0; res < MAX_MIPMAP; res++) {
        /* write each valid map down, if output is required
         * as twiddled, mess it up before saving to disk
         */
        if(m->map[res] != NULL) {
            if(use_twiddle)
                ok = write_twiddled(fp, cb, m, res);
            else
                ok = write_linear(fp, cb, m, res);

            if(ok < 0) {
                fprintf(stderr, "FATAL: error writing index data to %s\n", filename);
                goto loser;
            }
        }
    }

    fclose(fp);
    return 0;

loser:
    fclose(fp);
    unlink(filename);
    return -1;
}


static void banner(const char *progname) {
    printf("Usage: %s [options] image1 [image2..]\n", progname);
    printf("\n");
    printf("Options:\n");
    printf("\t-t, --twiddle\tcreate twiddled textures\n");
    printf("\t-m, --mipmap\tcreate mipmapped textures (EXPERIMENTAL)\n");
    printf("\t-v, --verbose\tverbose\n");
    printf("\t-d, --debug\tshow debug information\n");
    printf("\t-q, --highq\thigher quality (much slower)\n");
    printf("\t-k, --kmg\twrite a KMG for output\n");
    printf("\t-a, --alpha\tuse alpha channel (and output ARGB4444)\n");
    printf("\t-b, --amask\tuse 1-bit alpha mask (and output ARGB1555)\n");
}

static int mipmap_index(int s) {
    int mip;

    /* already assuming size is valid, so no funky business */
    mip = 0;

    /* no 1x1 bitmaps */
    s >>= 2;

    while(s) {
        s >>= 1;
        mip++;
    }

    return mip;
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

static fquad_t *create_map(int res, image_t *im) {
    int x, y;
    int nquads;
    fquad_t *q, *qt;

    if(use_debug) {
        printf("create_map(%d)\n", res);
    }

    nquads = quads_in_map(res);
    q = (fquad_t *)malloc(nquads * sizeof(fquad_t));

    if(q == NULL)
        return NULL;

    qt = q;

    for(y = 0; y < im->h; y += 2) {
        /* warning, ugly code coming up */
        for(x = 0; x < im->w; x += 2) {
            get_color(&qt->p[0], im->data + (y * im->stride) + (x * 4));
            get_color(&qt->p[1], im->data + (y * im->stride) + ((x + 1) * 4));
            get_color(&qt->p[2], im->data + ((y + 1)*im->stride) + (x * 4));
            get_color(&qt->p[3], im->data + ((y + 1)*im->stride) + ((x + 1) * 4));
            qt++;
        }
    }

    return q;
}

static void destroy_mipmap(mipmap_t *m) {
    int i;

    for(i = 0; i < MAX_MIPMAP; i++) {
        if(m->map[i]) {
            free(m->map[i]);
            m->map[i] = NULL;
        }
    }
}

static fquad_t *create_downscaled_map(int res, fquad_t *oneup) {
    int y, x, nquads, qw;
    fquad_t *q, *larger, tmp;

    if(use_debug) {
        printf("create_downscaled_map(%d %lx)\n", res, (uintptr_t)oneup);
    }

    /* each quad in the lower resolution is an average of
     * four quads in the higher resolution map.
     */

    qw = 1 << res;
    nquads = quads_in_map(res);
    q = (fquad_t *)malloc(sizeof(fquad_t) * nquads);

    if(q == NULL)
        return NULL;

    for(y = 0; y < qw; y++) {
        for(x = 0; x < qw; x++) {
            /* clear temporary quad */
            clear_quad(&tmp);

            larger = &oneup[y * 2 * qw * 2 + x * 2];
            sum_colors(&tmp.p[0], &larger->p[0]);
            sum_colors(&tmp.p[0], &larger->p[1]);
            sum_colors(&tmp.p[0], &larger->p[2]);
            sum_colors(&tmp.p[0], &larger->p[3]);
            div_colors(&tmp.p[0], 4.0f);

            larger = &oneup[y * 2 * qw * 2 + x * 2 + 1];
            sum_colors(&tmp.p[1], &larger->p[0]);
            sum_colors(&tmp.p[1], &larger->p[1]);
            sum_colors(&tmp.p[1], &larger->p[2]);
            sum_colors(&tmp.p[1], &larger->p[3]);
            div_colors(&tmp.p[1], 4.0f);

            larger = &oneup[(y * 2 + 1) * qw * 2 + x * 2];
            sum_colors(&tmp.p[2], &larger->p[0]);
            sum_colors(&tmp.p[2], &larger->p[1]);
            sum_colors(&tmp.p[2], &larger->p[2]);
            sum_colors(&tmp.p[2], &larger->p[3]);
            div_colors(&tmp.p[2], 4.0f);

            larger = &oneup[(y * 2 + 1) * qw * 2 + x * 2 + 1];
            sum_colors(&tmp.p[3], &larger->p[0]);
            sum_colors(&tmp.p[3], &larger->p[1]);
            sum_colors(&tmp.p[3], &larger->p[2]);
            sum_colors(&tmp.p[3], &larger->p[3]);
            div_colors(&tmp.p[3], 4.0f);

            copy_quad(&q[y * qw + x], &tmp);
        }
    }

    return q;
}

static int build_mipmap(mipmap_t *m, image_t *i) {
    int size;
    int image_res;

    if(use_verbose) {
        printf("o");
    }

    /* paranoia, so later you can use destory_mipmap safely */
    memset(m, '\0', sizeof(*m));

    image_res = mipmap_index(i->w);

    m->map[image_res] = create_map(image_res, i);

    if(m->map[image_res] == NULL)
        return -ENOMEM;

    if(use_mipmap) {
        /* create maps in lower resolution */
        size = image_res - 1;

        while(size >= 0) {
            m->map[size] = create_downscaled_map(size, m->map[size + 1]);
            size--;
        }
    }

    return 0;
}

static void place_quads(context_t *cb, mipmap_t *m) {
    int i, j, quads;

    /* run three times to get better quality;
     * this is not required for most of textures
     */

    reset_codebook(cb);

    for(j = 0; j < (use_hq ? 3 : 1); j++) {
        for(i = 0; i < MAX_MIPMAP; i++) {
            /* for each mipmap, scan all quads and update
             * statistics of which quad is the closest to
             * which codebook index entry
             */
            if(m->map[i] != NULL) {
                quads = quads_in_map(i);
                place(cb, m->map[i], quads);
            }
        }
    }

    clean_codebook(cb);
}


static const char *figure_outfilename(const char *f, const char *newext) {
    char *ext;
    size_t namelen;
    char *newname;

    ext = strrchr(f, '.');

    namelen = (ext == NULL) ? strlen(f) : (ext - f);

    newname = (char *)calloc(namelen + strlen(newext) + 2, sizeof(char));

    if(!newname) return NULL;

    strncpy(newname, f, namelen);
    strcat(newname, ".");
    strcat(newname, newext);

    return newname;
}

static void destroy_image(image_t *image) {
    if(image->data) {
        free(image->data);
        image->data = NULL;
    }
}

static int encode(const char *infile) {
    int     i, ok;
    image_t     image;
    mipmap_t    mipmap;
    context_t   context;
    const char  *outfile;

    if(use_verbose) {
        printf("encoding %s.. ", infile);
    }

    if(get_image(infile, &image) < 0) {
        fprintf(stderr, "failed reading %s\n", infile);
        return -EINVAL;
    }

    /* if (image.bpp != 3) {
        fprintf(stderr, "not a 24bit image (%d bits)\n", 8 * image.bpp);
        destroy_image(&image);
        return -EINVAL;
    } */

    if(image.w != image.h) {
        fprintf(stderr, "%s is not a square image\n", infile);
        destroy_image(&image);
        return -EINVAL;
    }

    if(valid_size(image.w) == 0) {
        fprintf(stderr, "image dimensions for %s are not valid, see manual\n", infile);
        destroy_image(&image);
        return -EINVAL;
    }

    if(use_kmg)
        outfile = figure_outfilename(infile, "kmg");
    else
        outfile = figure_outfilename(infile, "vq");

    if(outfile == NULL) {
        fprintf(stderr, "memory allocation failed for %s\n", infile);
        return -ENOMEM;
    }

    new_context(&context);
    build_mipmap(&mipmap, &image);

    /* feed all quads (all resolutions) */
    place_quads(&context, &mipmap);

    /* starting with one codebook entry, split 7 times */
    for(i = 1; i <= 7; i++) {
        if(use_verbose) {
            printf("o");
        }

        split(&context);
        place_quads(&context, &mipmap);
    }

    split(&context);

    if(use_verbose) {
        printf("\n");
    }

    ok = save(outfile, &context, &mipmap, &image);

    destroy_mipmap(&mipmap);
    destroy_image(&image);
    return ok;
}

static int process_long_options(char *arg) {
    if(! strcmp(arg, "mipmap"))
        use_mipmap = 1;
    else if(! strcmp(arg, "twiddle"))
        use_twiddle = 1;
    else if(! strcmp(arg, "debug"))
        use_debug = 1;
    else if(! strcmp(arg, "verbose"))
        use_verbose = 1;
    else if(! strcmp(arg, "highq"))
        use_hq = 1;
    else if(! strcmp(arg, "kmg"))
        use_kmg = 1;
    else if(! strcmp(arg, "alpha"))
        use_alpha = 1;
    else if(! strcmp(arg, "amask"))
        use_alpha = 2;
    else
        return -EINVAL;

    return 0;
}

static int process_option(char *arg) {
    /* assuming starts with '-' */
    arg++;

    switch(*arg) {

        case 'm':
            use_mipmap = 1;
            return 0;

        case 't':
            use_twiddle = 1;
            return 0;

        case 'v':
            use_verbose = 1;
            return 0;

        case 'd':
            use_debug = 1;
            return 0;

        case 'q':
            use_hq = 1;
            return 0;

        case 'k':
            use_kmg = 1;
            return 0;

        case 'a':
            use_alpha = 1;
            return 0;

        case 'b':
            use_alpha = 2;
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
