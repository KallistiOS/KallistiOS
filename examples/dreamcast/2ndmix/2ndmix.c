/* KallistiOS ##version##

   2ndmix.c
   (c)2000-2002 Megan Potter

*/

/*

The good ol' 2ndmix example that just won't die ^_^

This is probably the worst piece of code I've ever written and actually
kept around, but it's an awesome regression test... now ported to the
new PVR API.

*/

#include <kos.h>
#include <stdlib.h>
#include <assert.h>

/* Floating-point Sin/Cos; 256 angles, -1.0 to 1.0 */
#include "sintab.h"
#define msin(angle) sintab[angle]
#define mcos(angle) sintab[((angle)+64) % 256]

int randnum(int limit) {
    return (rand() & 0x7fffffff) % limit;
}

/* Texture offsets */
pvr_ptr_t txr_font = NULL;

/**********************************************************/
/* Load and start an S3M file from disk or memory */
#include "s3mplay.h"

volatile unsigned long *snd_dbg = (unsigned long*)0xa080ffc0;

void play_s3m(char *fn) {
    int idx, r;
    uint32 fd;
    unsigned char buffer[2048];

    spu_disable();

    printf("Loading %s\r\n", fn);
    fd = fs_open(fn, O_RDONLY);

    if(fd == 0) {
        printf("Can't open file %s\r\n", fn);
        return;
    }

    idx = 0x10000;

    /* Load 2048 bytes at a time */
    while((r = fs_read(fd, buffer, 2048)) > 0) {
        spu_memload(idx, buffer, r);
        idx += r;
    }

    fs_close(fd);

    /* Switch channels to mono if holding down A */
    /*if (check_for_btn(BTN_A)) {
        snd_dbg[1] = 1;
    } else {
        snd_dbg[1] = 0;
    }*/

    //dc_serial_printf("Load OK, starting ARM\r\n");
    printf("Loading ARM program\r\n");
    spu_memload(0, s3mplay, sizeof(s3mplay));

    printf("Start\r\n");
    spu_enable();

    while(*snd_dbg != 3)
        ;

    while(*snd_dbg == 3)
        ;

    printf("Done\r\n");
}

/**********************************************************/
/* 3D star code, written a long time ago =) */
#define NS 1024
int *star_x = NULL, *star_y, *star_z;

/* init stars so that they will be anywhere on the screen, but will start out
   as a single dot in the middle because of the Z coord.

   This routine has been mathematically optimized to produce the best possible
   spread of stars on the given starfield parameters by solving the
   perspective equation for X and Y. */

#define zk 64
#define MAXX ((640*(zk+640))/zk)
#define MAXY ((480*(zk+480))/zk)

pvr_poly_hdr_t stars_header;

void stars_init(void) {
    int i;
    pvr_poly_cxt_t tmp;

    if(star_x == NULL) {
        star_x = malloc(NS * sizeof(int));
        star_y = malloc(NS * sizeof(int));
        star_z = malloc(NS * sizeof(int));
    }

    for(i = 0; i < NS; i++) {
        star_x[i] = randnum(MAXX) - (MAXX / 2);
        star_y[i] = randnum(MAXY) - (MAXY / 2);
        star_z[i] = randnum(640);
    }

    pvr_poly_cxt_col(&tmp, PVR_LIST_OP_POLY);
    pvr_poly_compile(&stars_header, &tmp);
}

/* Draws a point using a triangle strip */
void poly_pnt(int x, int y, float z, float size, int color) {
    pvr_vertex_t vert;

    vert.flags = PVR_CMD_VERTEX;
    vert.x = x;
    vert.y = y + size;
    vert.z = z;
    vert.u = vert.v = 0.0f;
    vert.argb = PVR_PACK_COLOR(1.0f, color / 256.0f, color / 256.0f, color / 256.0f);
    vert.oargb = 0;
    pvr_prim(&vert, sizeof(vert));

    vert.y = y;
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = x + size;
    vert.y = y + size;
    pvr_prim(&vert, sizeof(vert));
}

void stars_one_frame(void) {
    int i, x1, y1, xn, yn, zn, c;

    /* Send polygon header to the TA using store queues */
    pvr_prim(&stars_header, sizeof(stars_header));

    /* Calculate each star's position and plot it on screen */
    for(i = 0; i < NS; i++) {
        /* Calculate star perspective */
        xn = star_x[i];
        yn = star_y[i];
        zn = star_z[i];
        x1 = xn * zk / (zn + zk);
        y1 = yn * zk / (zn + zk);

        if(x1 > -320 && y1 > -240 && x1 < 320 && y1 < 240) {
            c = (zn * 256 / 640) & 0xff;
            poly_pnt(x1 + 320, y1 + 240, 1.0f, 4.0f, 255 - c);
        }
        else {
            star_z[i] = 640;
        }

        /* Move star's Z coord to show motion inward */
        star_z[i] -= 8;
    }
}

/********************************************************************/
/* Draws some nifty 3D cubes on the screen */

/* Cube data is now an ordered list of triangle strips */
#define NUM_FACES 6
int cube_points[] = {
    -1, -1,  1, /* Front plane */
    -1,  1,  1,
    1, -1,  1,
    1,  1,  1,

    1, -1, -1, /* Back plane */
    1,  1, -1,
    -1, -1, -1,
    -1,  1, -1,

    -1, -1, -1, /* Left plane */
    -1,  1, -1,
    -1, -1,  1,
    -1,  1,  1,

    1, -1,  1, /* Right plane */
    1,  1,  1,
    1, -1, -1,
    1,  1, -1,

    -1, -1, -1, /* Top plane */
    -1, -1,  1,
    1, -1, -1,
    1, -1,  1,

    -1,  1,  1, /* Bottom plane */
    -1,  1, -1,
    1,  1,  1,
    1,  1, -1
};

int rotang = 0;
#define zkonst 128
#define mult 64
float cubesizes[6] = { 0.0f };
int tmpframe = 0;
void draw_cube(int which) {
    int cp;
    int ra, ra2;
    float x, y, z, tx, ty, tz;
    float xt, yt;
    float xtrans, ytrans, ztrans;
    float a, r, g, b;
    pvr_vertex_t vert;

    cp = 0;

    if(cubesizes[which] < 0.1f)
        return;

    ra = (rotang * 2) % 256;
    ra = (ra + 42 * which) % 256; /* heh */
    ra2 = (rotang / 2);

    /* If the current parameters don't make you motion sick,
       then try setting *32 on ytrans to something higher =) */
    xtrans = mcos(ra) * 220;
    ytrans = msin((ra * 3) % 256) * 32;
    ztrans = msin(ra) * 256 + 256.0f;

    for(cp = 0; cp < (NUM_FACES * 4 * 3); cp += 3) {
        x = cube_points[cp + 0] * mult * cubesizes[which];
        y = cube_points[cp + 1] * mult * cubesizes[which];
        z = cube_points[cp + 2] * mult * cubesizes[which];

        tx = (mcos(ra) * x - msin(ra) * y);
        ty = (mcos(ra) * y + msin(ra) * x);
        x = tx;
        y = ty;

        tz = (mcos(ra2) * z - msin(ra2) * y);
        ty = (mcos(ra2) * y + msin(ra2) * z);
        y = ty;
        z = tz;

        tx = (mcos(ra) * x - msin(ra) * z);
        tz = (mcos(ra) * z + msin(ra) * x);
        x = tx;
        z = tz;

        z += ztrans;

        xt = (x * zkonst) / (zkonst + z + mult);
        yt = (y * zkonst) / (zkonst + z + mult);

        xt += xtrans;
        yt += ytrans;

        if((cp % 12) == 9)
            vert.flags = PVR_CMD_VERTEX_EOL;
        else
            vert.flags = PVR_CMD_VERTEX;

        vert.x = xt + 320.0f;
        vert.y = yt + 240.0f;
        vert.z = 128.0f - (z / 64.0f);
        a = 0.8f;

        if(ztrans == 0) ztrans++;

        switch((cp % 4)) {
            case 0:
                r = 192.0f / ztrans;
                g = 0.0f;
                b = 0.0f;
                break;
            case 1:
                r = 0.0f;
                g = 192.0f / ztrans;
                b = 0.0f;
                break;
            case 2:
                r = 0.0f;
                g = 0.0f;
                b = 192.0f / ztrans;
                break;
            case 3:
                r = 192.0f / ztrans;
                g = 192.0f / ztrans;
                b = 0.0f;
                break;
            default:
                assert(0);
                break;
        }

#define bound(A) if ((A) < 0.0f) (A) = 0.0f; if ((A) > 1.0f) (A) = 1.0f;
        bound(r);
        bound(g);
        bound(b);
#undef bound
        vert.argb = PVR_PACK_COLOR(a, r, g, b);
        vert.oargb = 0;
        vert.u = vert.v = 0.0f;

        pvr_prim(&vert, sizeof(vert));
    }
}

/* Draw six cubes arranged in a circle */
int zooming = 1;
void cube_one_frame(void) {
    int i, j;

    rotang = (rotang + 1) % 512;

    if(zooming) {
        for(j = 0; j < 6; j++) {
            if(cubesizes[j] < 1.0f) {
                cubesizes[j] += 1 / 85.0f;
                break;
            }
            else {
                cubesizes[j] = 1.0f;
            }
        }

        if(j >= 6)
            zooming = 0;
    }

    for(i = 0; i < 6; i++) {
        draw_cube(i);
    }
}

int cubes_have_header = 0;
pvr_poly_hdr_t cubes_header;

void cubes_one_frame(void) {
    pvr_poly_cxt_t tmp;

    if(!cubes_have_header) {
        pvr_poly_cxt_col(&tmp, PVR_LIST_TR_POLY);
        pvr_poly_compile(&cubes_header, &tmp);
        cubes_have_header = 1;
    }

    /* Send polygon header to the TA using store queues */
    pvr_prim(&cubes_header, sizeof(cubes_header));

    /* Draw cubes */
    cube_one_frame();
}

/**********************************************************/
/* Port of an OLD OLD PCX loading function =) */
/* I should really convert this to use the new image API, but we need
   a few more support bits in there before this will work fully. */

struct pcx_hdr {
    char   Mfg;               // manufacturer, always 0xa0
    char   Ver;               // encoder version number (5)
    char   Enc;               // encoding code, always 1
    char   Bpp;               // bits per pixel, 8 in mode 0x13
    uint16 Xmin, Ymin;        // image origin, usually 0,0
    uint16 Xmax, Ymax;        // image dimensions
    uint16 Hres;              // horizontal resolution value
    uint16 Vres;              // vertical resolution value
    char   Pal[48];           // palette (not in mode 0x13)
    char   Reserved;          // who knows?
    char   ClrPlanes;         // number of planes, 1 in mode 0x13
    uint16 Bpl;               // bytes per line, 80 in mode 0x13
    uint16 plType;            // Grey or Color palette flag
    char   Filler[58];        // Zsoft wanted a 128 byte header
};

short   imageWidth, imageHeight;
uint8   *image;         /* Allocate this first */
uint8   *pcxpal;
char    *charmap;

int load_pcx(char *pcxdata) {
    int bytes;         // counts unpacked bytes
    char    c;         // byte being processed
    int runlen;        // length of packet
    int num_bytes;
    struct  pcx_hdr pcxh;

    if(image == NULL) {
        printf("Image not allocated\r\n");
        return 0;
    }

    bytes = 0;

    memcpy(&pcxh, pcxdata, sizeof(pcxh));
    pcxdata += sizeof(pcxh);

    if(pcxh.Bpp != 8) {
        printf("PCX data is not 8bpp\r\n");
        return 0;
    }

    imageWidth = pcxh.Xmax + 1;
    imageHeight = pcxh.Ymax + 1;
    num_bytes = imageWidth * imageHeight;
    printf("Image is %dx%d (%d bytes)\r\n", imageWidth, imageHeight, num_bytes);

    do {
        c = *(pcxdata++);

        if((c & 0xc0) == 0xc0) {    // high 2 bits set is packet
            runlen = (c & 0x3f);      // AND off the high bits
            c = *(pcxdata++);

            while(runlen--)(image[bytes++] = c);
        }
        else
            image[bytes++] = c;
    }
    while(bytes < num_bytes);

    pcxdata++;                // this is a marker before the palette
    memcpy(pcxpal, pcxdata, 768);
    pcxdata += 768;

    return 1;
}

/**********************************************************/
/* Scrolly routines */

#include "gfx/font14_256.h"
#define FONT_NAME font14_256_pcx
#define FONT_WIDTH 8
#define FONT_HEIGHT 9
#define FONT_PIC_WIDTH 256
#define FONT_PIC_HEIGHT 256
#define FONT_DOUBLE 2.2
char font_map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789?:;/!\"#$%&'[]*+,-."
                  "abcdefghijklmnopqrstuvwxyz^_@ ";
#define FONT_HAS_LC

/*
#include "gfx/font5_256.h"
#define FONT_NAME font5_256_pcx
#define FONT_WIDTH 16
#define FONT_HEIGHT 20
#define FONT_PIC_WIDTH 256
#define FONT_PIC_HEIGHT 256
#define FONT_DOUBLE 1
char font_map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+#!*:.,\\?-+=@$%() ";
#undef FONT_HAS_LC
*/

#define FONT_PERROW (FONT_PIC_WIDTH / FONT_WIDTH)
#define FONT_ROWS (480 / FONT_HEIGHT)

typedef struct {
    float   x, y, fade;
    char    *chars;
} fontrow_t;
fontrow_t font_rows[FONT_ROWS];

char *scrollypos = NULL;
char scrolly[] =
    "\x02Stars:2ndMix\0"
    "By Megan Potter\0"
    "\r\0"

    "\x0c"
    "Hi there.. welcome to the second\0"
    "released demo for the Dreamcast\0"
    "video game system. It's not quite\0"
    "so simple anymore. :]\0"
    "\0"
    "In the background you can see a\0"
    "simple star effect, playing over\0"
    "an S3M by Krystall. Krystall was\0"
    "a good friend in the days of\0"
    "Digital Infinity so I hope he'll\0"
    "forgive me for using his\0"
    "track..:]\0"
    "\r\0"

    "\x0d"
    "You can also see a little cube\0"
    "flying around back there. At long\0"
    "last we can make use of the 3D\0"
    "acceleration hardware in the DC.\0"
    "This demo has come a long way since\0"
    "its humble beginnings.\0"
    "\0"
    "The music is running entirely\0"
    "in the dedicated sound CPU, using\0"
    "an S3M player I wrote way back\0"
    "when. It takes no main processor\0"
    "cycles. Pretty cool, huh?\0"
    "\r\0"

    "\x0c"
    "The source code to this demo\0"
    "and things to help you make\0"
    "your own cool demos and games\0"
    "can be found here:\0"
    "\0"
    "http://dcdev.allusion.net/\0"
    "\0"
    "There is NO Katana code in this\0"
    "program or any others on my site.\0"
    "Rather, it uses the new Cryptic\0"
    "Allusion DC operating system,\0"
    "KallistiOS.\0"
    "\r\0"

    "\x09"
    "Quick hellos to the people out\0"
    "in the dcdev 'scene' and the\0"
    "folks at Sega [we love your\0"
    "machine, sorry it didn't work\0"
    "out for you].\0"
    "\0"
    "Well.. the other effects never\0"
    "got ported so here is where it\0"
    "wraps. Hit START to exit!\0"
    "\a\0";
int active_lines = 0, pause_count = 0, mode = 0;

/* Draws a font letter using two triangle strips */
void font_draw_char(int x1, int y1, float color, int ch) {
    int index, ix, iy;
    float   u1, v1, u2, v2;
    pvr_vertex_t    vert;

    index = charmap[ch];
    ix = (index % FONT_PERROW) * FONT_WIDTH;
    iy = (index / FONT_PERROW) * FONT_HEIGHT;
    u1 = ix * 1.0f / FONT_PIC_WIDTH;
    v1 = iy * 1.0f / FONT_PIC_HEIGHT;
    u2 = (ix + FONT_WIDTH) * 1.0f / FONT_PIC_WIDTH;
    v2 = (iy + FONT_HEIGHT) * 1.0f / FONT_PIC_HEIGHT;

    if(index == -1)
        return;

    vert.flags = PVR_CMD_VERTEX;
    vert.x = x1;
    vert.y = y1 + FONT_HEIGHT * FONT_DOUBLE;
    vert.z = 512.0f;
    vert.u = u1;
    vert.v = v2;
    /* if (color > 1.0f) color = 1.0f;
    if (color < 0.0f) color = 0.0f; */
    vert.argb = PVR_PACK_COLOR(color, 0.0f, 0.75f, 1.0f);
    vert.oargb = 0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = x1;
    vert.y = y1;
    vert.u = u1;
    vert.v = v1;
    pvr_prim(&vert, sizeof(vert));

    vert.x = x1 + FONT_WIDTH * FONT_DOUBLE;
    vert.y = y1 + FONT_HEIGHT * FONT_DOUBLE;
    vert.u = u2;
    vert.v = v2;
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = x1 + FONT_WIDTH * FONT_DOUBLE;
    vert.y = y1;
    vert.u = u2;
    vert.v = v1;
    pvr_prim(&vert, sizeof(vert));
}

void font_draw_string(int x1, int y1, float color, char *str) {
    while(*str) {
        font_draw_char(x1, y1, color, *str++);
        x1 += (int)(FONT_WIDTH * FONT_DOUBLE + 1);
    }
}

void font_next_screen(void) {
    int width;
    int y, y1, x;

    if(!scrollypos)
        scrollypos = scrolly;

    memset(font_rows, 0, sizeof(font_rows));

    active_lines = *scrollypos++;
    y1 = (480 - active_lines * (FONT_HEIGHT * FONT_DOUBLE + 4)) / 2;
    y = 0;

    while(*scrollypos != '\r' && *scrollypos != '\a') {
        x = strlen(scrollypos);
        width = x * (int)(FONT_WIDTH * FONT_DOUBLE + 1);

        font_rows[y].x = (640 - width) / 2;
        font_rows[y].y = y1;
        font_rows[y].chars = scrollypos;
        font_rows[y].fade = 0.0f;
        scrollypos += x + 1;
        y1 += (int)(FONT_HEIGHT * FONT_DOUBLE + 4);
        y++;
    }

    if(*scrollypos == '\a') {
        scrollypos = scrolly;
    }
    else {
        scrollypos += 2;
    }
}

#if 0
/* Debug code to draw the font texture */
void blit_font_texture(void) {
    vertex_ot_t vert;

    vert.flags = TA_VERTEX_NORMAL;
    vert.x = 50.0f;
    vert.y = 50.0f + 512.0f;
    vert.z = 512.0f;
    vert.u = 0.0f;
    vert.v = 1.0f;
    vert.a = 0.6f;
    vert.r = 1.0f;
    vert.g = 1.0f;
    vert.b = 1.0f;
    vert.oa = vert.or = vert.og = vert.ob = 0.0f;
    ta_commit_vertex(&vert, sizeof(vert));

    vert.x = 50.0f;
    vert.y = 50.0f;
    vert.u = 0.0f;
    vert.v = 0.0f;
    ta_commit_vertex(&vert, sizeof(vert));

    vert.x = 50.0f + 512.0f;
    vert.y = 50.0f + 512.0f;
    vert.u = 1.0f;
    vert.v = 1.0f;
    ta_commit_vertex(&vert, sizeof(vert));

    vert.flags = TA_VERTEX_EOL;
    vert.x = 50.0f + 512.0f;
    vert.y = 50.0f;
    vert.u = 1.0f;
    vert.v = 0.0f;
    ta_commit_vertex(&vert, sizeof(vert));

    return;
}
#endif

pvr_poly_hdr_t  font_header;

void font_one_frame(void) {
    int     done, y, actrows;

    actrows = 0;

    /* Start a textured polygon set (with the font texture) */
    pvr_prim(&font_header, sizeof(font_header));

    //blit_font_texture();
    //return;

    /* Draw all font rows */
    for(y = 0; y < FONT_ROWS; y++) {
        if(font_rows[y].fade >= 0.05f && font_rows[y].chars != NULL) {
            font_draw_string(font_rows[y].x, font_rows[y].y,
                             font_rows[y].fade, font_rows[y].chars);
            actrows++;
        }
    }

    /* Do any pausing */
    if(pause_count > 0) {
        pause_count--;
        return;
    }

    if(!mode) {
        done = 1;

        for(y = 0; y < FONT_ROWS; y++) {
            if(font_rows[y].fade < 1.0f) {
                font_rows[y].fade += 0.05f;
                done = 0;
            }
        }

        if(done) {
            mode = 1;
            pause_count = actrows * 60;
        }
    }
    else {
        done = 1;

        for(y = 0; y < FONT_ROWS; y++) {
            if(font_rows[y].chars != NULL && font_rows[y].fade >= 0.1f) {
                font_rows[y].fade -= 0.1f;
                done = 0;
            }
        }

        if(done) {
            mode = 0;
            pause_count = 60;
            font_next_screen();
        }
    }
}

void font_init(void) {
    size_t x, y, c;
    uint8 pcxpall[768];
    volatile uint16 *vtex;
    uint16 val;
    pvr_poly_cxt_t tmp;

    txr_font = pvr_mem_malloc(256 * 256 * 2);
    vtex = (vuint16*)txr_font;

    charmap = malloc(256);
    image = malloc(FONT_PIC_WIDTH * FONT_PIC_HEIGHT);
    pcxpal = pcxpall;

    if(!load_pcx((char *)FONT_NAME)) {
        printf("Couldn't load PCX of font image\r\n");
        return;
    }

    printf("Drawing into 0x%08lx\r\n", (uint32)vtex);

    for(y = 0; y < FONT_PIC_HEIGHT; y++) {
        for(x = 0; x < FONT_PIC_WIDTH; x++) {
            val = pcxpal[image[y * 256 + x] * 3];

            if(val != 0)
                val = ((val >> 4) << 8)
                      | ((val >> 4) << 4)
                      | ((val >> 4) << 0)
                      | 0xf000;

            *vtex++ = val;
        }
    }

    for(c = 0; c < 255; c++)
        charmap[c] = -1;

    for(c = 0; c < strlen(font_map); c++) {
        charmap[(int)font_map[c]] = c;
#ifndef FONT_HAS_LC
        charmap[(int)tolower(font_map[c])] = c;
#endif
    }

    charmap[' '] = -1;

    memset(font_rows, 0, sizeof(font_rows));

    font_next_screen();

    free(image);

    /* Make a polygon header */
    pvr_poly_cxt_txr(&tmp, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB4444 | PVR_TXRFMT_NONTWIDDLED,
                     256, 256, txr_font, PVR_FILTER_NONE);
    pvr_poly_compile(&font_header, &tmp);
}

/********************************************************************/

int framecnt = 0;
void draw_one_frame(void) {
    /* Begin opaque polygons */
    pvr_wait_ready();
    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_OP_POLY);

    /* Draw stars */
    stars_one_frame();

    /* Begin translucent polygons */
    pvr_list_finish();
    pvr_list_begin(PVR_LIST_TR_POLY);

    /* Draw cubes */
    cubes_one_frame();

    /* Draw scrolly */
    font_one_frame();

    /* Finish up */
    pvr_list_finish();
    pvr_scene_finish();

    framecnt++;
}

pvr_init_params_t params = {
    /* Enable opaque and translucent polygons with size 16 */
    { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0 },

    /* Vertex buffer size 512K */
    512 * 1024,

    /* Defaults for the rest */
    0, 0, 0, 0, 0
};

KOS_INIT_FLAGS(INIT_DEFAULT);

/* Main program: init and loop drawing polygons */
int main(int argc, const char *argv[]) {
    maple_device_t *cont;
    cont_state_t *state;

    printf("2ndMix/KallistiOS starting\n");

    printf("Initializing new PVR system\n");
    pvr_init(&params);

    printf("Initializing stars\n");
    stars_init();
    printf("Init font\n");
    font_init();

    printf("Loading music\n");
    play_s3m("/rd/e-79014.s3m");

    printf("Starting display\n");

    while(1) {
        if((cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER)) != NULL) {
            state = (cont_state_t *)maple_dev_status(cont);

            if(state == NULL)
                break;

            if(state->buttons & CONT_START)
                break;
        }

        draw_one_frame();
    }

    /* Clean up what we allocated */
    free(charmap);
    free(star_x);
    free(star_y);
    free(star_z);

    printf("Done, returning\n");
    return 0;
}


