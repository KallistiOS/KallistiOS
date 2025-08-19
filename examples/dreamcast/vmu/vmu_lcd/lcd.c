/*  KallistiOS ##version##

    lcd.c
    Copyright (C) 2023 Paul Cercueil

*/

/*
    This example demonstrates drawing dynamic contents to the
    VMU's LCD display. It does so by rendering to a virtual
    framebuffer and then presenting it, which sends the updated
    framebuffer to the VMU over the maple protocol which displays
    it.

    This demo also shows off rendering dynamic text using an
    embedded font.
 */

#include <stdio.h>
#include <math.h>
#include <stdint.h>

#include <kos/init.h>

#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/vmu.h>
#include <dc/vmu_fb.h>
#include <dc/fmath.h>

#include <arch/arch.h>

static const uint8_t smiley[] = {
    0b00111100,
    0b01000010,
    0b10100101,
    0b10000001,
    0b10100101,
    0b10011001,
    0b01000010,
    0b00111100,
};

static vmufb_t vmufb;
static const char message[] = "        Hello World!        ";

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

/* Your program's main entry point */
int main(int argc, char **argv) {
    unsigned int x, y, i, vmu;
    maple_device_t *dev;
    const vmufb_font_t *font;
    float val;

    /* If start is pressed, exit the app. */
    cont_btn_callback(0, CONT_START,
                      (cont_btn_callback_t)arch_exit);

    font = vmu_get_font();

    for(i = 0; ; i++) {
        vmufb_clear(&vmufb);

        val = (float)i * F_PI / 360.0f;
        x = 20 + (int)(20.0f * cosf(val));
        y = 12 + (int)(12.0f * sinf(val));

        vmufb_paint_area(&vmufb, x, y, 8, 8, smiley);

        vmufb_print_string_into(&vmufb, font,
                    12, 12, 24, 6, 0,
                    &message[(i / 16) % sizeof(message)]);


        for(vmu = 0; !!(dev = maple_enum_type(vmu, MAPLE_FUNC_LCD)); vmu++) {
            vmufb_present(&vmufb, dev);
        }

        /* Now sleep for a bit so we can actually see the new frame */
        usleep(2000);
    }

    return 0;
}
