
/* KallistiOS 2.0.0

   pogo.c
   (c)2002 Gil Megidish

*/

/*

This is an inefficient example of how to get your KOS GBA code kicking; it
shows Commander Keen (animation by Blackeye Software,) jumping around with
his pogo stick.

Absolutely not the way to write games; but this is just to show basic
guidelines of initializing and using the romdisk filesystem with the gba,
and the use of video registers directly.

*/

#include <gba/video.h>
#include <gba/keys.h>
#include <kos.h>
#include <kos/pcx.h>

extern uint8 romdisk[];

void arm_irq_handler() { }

static uint16 pogo_bitmaps[7][48 * 48];

static uint16 *get_pogo_bitmap(int index) {
    return pogo_bitmaps[index];
}

void gba_setmode(int mode, int bg) {
    REG_DISPCNT = mode | (1 << (bg + 8));
}

static int sequence = 0;
static int pogo_x = (240 + 48) / 2;
static int pogo_y = (160 - 48);
static int pogo_xmax = 240 - 48 - 1;
static int pogo_ymax = 160 - 48;
static int pogo_dir = 0;

#define RGB16(r,g,b) ((r>>3) | ((g>>3) << 5) | ((b>>3) << 10))

static void draw_bitmap(uint16 *rgb) {
    /* this will draw a 48x48 rgb bitmap onto a 16bit video buffer */
    uint16 color;
    uint16 *ptr;
    int direction;
    int i, j;

    ptr = (uint16*)PTR_VID_0 + (pogo_y * 240) + pogo_x;

    if(pogo_dir == 0)
        direction = 1;
    else {
        /* facing right */
        direction = -1;
        ptr = ptr + 48;
    }

    for(j = 0; j < 48; j++) {
        for(i = 0; i < 48; i++) {
            color = *rgb++;

            if(color == RGB16(0xff, 0, 0xff))
                color = 0;

            *ptr = color;
            ptr = ptr + direction;
        }

        ptr = ptr + 240 - (48 * direction);
    }
}

static void animate() {
    uint16 *bitmap;

    bitmap = get_pogo_bitmap(sequence);
    draw_bitmap(bitmap);
}

static void wait() {
    volatile uint32 *status;

    /* wait for a full vsync */
    status = (uint32*)0x4000004;

    while((*status & 1) == 0);

    while((*status & 1) == 1);
}

static void dumb_sleep(int n) {
    while(n > 0) {
        wait();
        --n;
    }
}

static void check_keys() {
    int keys;

    keys = REG_KEYS;

    if((keys & KEY_LEFT) == 0) {
        /* left */
        --pogo_x;

        if(pogo_x < 0)
            pogo_x = 0;

        pogo_dir = 0;
    }

    if((keys & KEY_RIGHT) == 0) {
        /* right */
        ++pogo_x;

        if(pogo_x > pogo_xmax)
            pogo_x = pogo_xmax;

        pogo_dir = 1;
    }

    if((keys & KEY_UP) == 0) {
        /* up */
        --pogo_y;

        if(pogo_y < 0)
            pogo_y = 0;
    }

    if((keys & KEY_DOWN) == 0) {
        /* down */
        ++pogo_y;

        if(pogo_y > pogo_ymax)
            pogo_y = pogo_ymax;
    }
}

static void prepare() {
    int w, h;

    /* load all 7 bitmaps */
    pcx_load_flat("/rd/1.pcx", &w, &h, get_pogo_bitmap(0));
    pcx_load_flat("/rd/2.pcx", &w, &h, get_pogo_bitmap(1));
    pcx_load_flat("/rd/3.pcx", &w, &h, get_pogo_bitmap(2));
    pcx_load_flat("/rd/4.pcx", &w, &h, get_pogo_bitmap(3));
    pcx_load_flat("/rd/5.pcx", &w, &h, get_pogo_bitmap(4));
    pcx_load_flat("/rd/6.pcx", &w, &h, get_pogo_bitmap(5));
    pcx_load_flat("/rd/7.pcx", &w, &h, get_pogo_bitmap(6));
}

KOS_INIT_FLAGS(INIT_DEFAULT);
KOS_INIT_ROMDISK(romdisk);

int main() {
    /* initialize romdisk filesystem */

    /* initialize video mode (raw, 16 bit) */
    gba_setmode(3, 2);

    memset(PTR_VID_0, 0, 240 * 160);

    /* preload pogo animation */
    prepare();

    /* main loop */
    sequence = 0;

    while(REG_KEYS & KEY_A) {

        /* show current frame of animation, and sleep for some period */
        animate();
        dumb_sleep(2);

        /* next frame in animation (only 7 pictures in set) */
        sequence++;
        sequence = sequence % 7;

        /* probe keypad */
        check_keys();
    }

    /* go back to neverever land */
    arch_reboot();

    /* keep compiler happy(R) */
    return 0;
}
