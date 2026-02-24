/*  KallistiOS ##version##

    rumble.c
    Copyright (C) 2004 SinisterTengu
    Copyright (C) 2008, 2023, 2025 Donald Haase
    Copyright (C) 2024, 2025 Daniel Fairchild

*/

/*
        This example allows you to send raw commands to the rumble accessory
   (aka purupuru).

        This is a recreation of an original posted by SinisterTengu in 2004
   here: https://dcemulation.org/phpBB/viewtopic.php?p=490067#p490067 .
   Unfortunately, that one is lost, but I had based my vmu_beep testing on it,
   and the principle is the same. In each, a single 32-bit value is sent to the
   device which defines the features of the rumbling.

        TODO: This should be updated at some point to display and work from the
   macros in dc/maple/purupuru.h that define the characteristics of the raw
   32-bit value.

 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include <kos/init.h>

#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/purupuru.h>

#include <dc/minifont.h>
#include <dc/video.h>

KOS_INIT_FLAGS(INIT_DEFAULT);

typedef struct {
    purupuru_effect_t effect;
    const char *description;
} baked_pattern_t;

static const baked_pattern_t catalog[] = {
    {.effect = {.cont = false, .motor = 1, .fpow = 7, .freq = 26, .inc =  1}, .description = "Basic Thud (simple .5s jolt)"},
    {.effect = {.cont = true,  .motor = 1, .fpow = 1, .freq =  7, .inc = 49}, .description = "Car Idle (69 Mustang)"},
    {.effect = {.cont = false, .motor = 1, .fpow = 7, .conv = true, .freq = 21, .inc = 38}, .description = "Car Idle (VW beetle)"},
    {.effect = {.cont = false, .motor = 1, .fpow = 7, .conv = true, .freq = 57, .inc = 51}, .description = "Eathquake (Vibrate, and fade out)"},
    {.effect = {.cont = true,  .motor = 1, .fpow = 1, .freq =  40, .inc = 5}, .description = "Helicopter"},
    {.effect = {.cont = false, .motor = 1, .fpow = 2, .freq =  7, .inc = 0}, .description = "Ship's Thrust (as in AAC)"},
};

static const char *fieldnames[] = {"cont", "res",  "motor", "bpow", "div",
                                   "fpow", "conv", "freq",  "inc"
                                  };
static const int num_fields = sizeof(fieldnames) / sizeof(fieldnames[0]);

/* motor cannot be 0 (will generate error on official hardware), but we can set
 * everything else to 0 for stopping */
static const purupuru_effect_t rumble_stop = {.motor = 1};
static purupuru_effect_t effect = rumble_stop;

static int cursor_pos = 0;
static size_t catalog_index = 0;
static int loaded_pattern = -1;

static inline uint8_t offset2field(int offset) {
    switch (offset) {
        case 0:
            return effect.cont;  // cont

        case 1:
            return effect.res;  // res

        case 2:
            return effect.motor;  // motor

        case 3:
            return effect.bpow;  // bpow

        case 4:
            return effect.div;  // div

        case 5:
            return effect.fpow;  // fpow

        case 6:
            return effect.conv;  // conv

        case 7:
            return effect.freq;  // freq

        case 8:
            return effect.inc;  // inc

        default:
            return -1;
    }
}
static inline void alter_field_at_offset(int offset, int delta) {
    switch (offset) {
        case 0:
            effect.cont = !effect.cont;  // cont
            break;

        case 1:
            break;  // res (reserved, cannot be changed)

        case 2:
            effect.motor = (effect.motor + delta) & 0xf;  // motor

            if (effect.motor == 0) effect.motor = 1;  // motor cannot be zero

            break;

        case 3:
            effect.bpow = (effect.bpow + delta) & 0x7;  // bpow

            if (effect.bpow)
                effect.fpow = 0;  // cannot have both forward and backward power

            break;

        case 4:
            effect.div = !effect.div;  // div

            if (effect.conv && effect.div)
                effect.conv =
                    false;  // cannot have both convergent and divergent

            break;

        case 5:
            effect.fpow = (effect.fpow + delta) & 0x7;  // fpow

            if (effect.fpow)
                effect.bpow = 0;  // cannot have both forward and backward power

            break;

        case 6:
            effect.conv = !effect.conv;  // conv

            if (effect.conv && effect.div)
                effect.div =
                    false;  // cannot have both convergent and divergent

            break;

        case 7:
            effect.freq = (effect.freq + delta) & 0xff;  // freq
            break;

        case 8:
            effect.inc = (effect.inc + delta) & 0xff;  // inc
            break;

        default:
            break;
    }
}

void redraw_screen() {
#define STRBUFSIZE 64
    char str_buffer[STRBUFSIZE];
    int textpos_x = 128, textpos_y = 32;

    /* Start drawing and draw the header */
    vid_clear(0, 0, 0);
    minifont_set_color(0xff, 0xc0, 0x10); /* gold */
    minifont_draw_str(vram_s + (640 * textpos_y) + textpos_x, 640,
                      "Rumble Accessory Tester");

    /* Start drawing the changeable section of the screen */
    textpos_y += 30;
    textpos_x = 10;
    minifont_set_color(0, 0, 255); /* Blue */

    for (int i = 0; i < num_fields; i++)
        minifont_draw_str(vram_s + (640 * textpos_y) + (textpos_x + 60 * i),
                          640, fieldnames[i]);

    textpos_y += 16;

    for (int i = 0; i < num_fields; i++) {
        if (cursor_pos == i)
            minifont_set_color(255, 0, 0); /* Red */
        else
            minifont_set_color(255, 255, 255); /* White */

        snprintf(str_buffer, STRBUFSIZE, " %u ", offset2field(i));
        minifont_draw_str(vram_s + (640 * textpos_y) + (textpos_x + (60 * i)),
                          640, str_buffer);
    }

    textpos_y += 20;
    textpos_x = 10;
    minifont_set_color(255, 255, 255); /* White */
    minifont_draw_str(vram_s + (640 * textpos_y) + textpos_x, 640,
                      "effect hex value:");
    minifont_set_color(255, 0, 255); /* Magenta */
    snprintf(str_buffer, STRBUFSIZE, "0x%08lx", effect.raw);
    minifont_draw_str(vram_s + (640 * textpos_y) + textpos_x + 145, 640,
                      str_buffer);

    textpos_y += 32;
    minifont_set_color(255, 255, 255); /* White */
    minifont_draw_str(vram_s + (640 * textpos_y) + textpos_x, 640,
                      "Field description:");
    minifont_set_color(255, 0, 0); /* RED */
    minifont_draw_str(vram_s + (640 * textpos_y) + textpos_x + 160, 640, "[");
    minifont_draw_str(vram_s + (640 * textpos_y) + textpos_x + 160 + 8, 640,
                      fieldnames[cursor_pos]);
    minifont_draw_str(vram_s + (640 * textpos_y) + textpos_x +
                      (strlen(fieldnames[cursor_pos]) + 1) * 8 + 160,
                      640, "]");
    textpos_y += 16;
    textpos_x += 20;
    minifont_set_color(255, 255, 255); /* White */
    const char *field_descriptions[] = {
        // note that each description is 2 lines, some empty
        "Continuous Vibration. When set vibration will continue until stopped",
        "",

        "Reserved. Always 0s",
        "also will not be shown.",

        "Motor number. 0 will cause an error. 1 is the typical setting. "
        "4-bits.",
        "",

        "Backward direction (- direction) intensity setting bits.",
        "0 stops vibration. Exclusive with .fpow. Field is 3-bits.",

        "Divergent vibration. Make the rumble stronger until it stops.",
        "Exclusive with .conv.",

        "Forward direction (+ direction) intensity setting bits.",
        "0 stops vibration. Exclusive with .bpow. Field is 3-bits.",

        "Convergent vibration. Make the rumble weaker until it stops.",
        "Exclusive with .div.",

        "Vibration frequency. For most purupuru the range is 4-59. Field is "
        "8-bits.",
        "",

        "Vibration inclination period setting bits. Field is 8-bits.",
        "",

        "Setting .inc == 0 when .conv or .div are set results in error.",
        ""
    };
    minifont_draw_str(vram_s + (640 * textpos_y) + textpos_x, 640,
                      field_descriptions[cursor_pos * 2]);
    textpos_y += 16;
    minifont_draw_str(vram_s + (640 * textpos_y) + textpos_x, 640,
                      field_descriptions[cursor_pos * 2 + 1]);

    if (loaded_pattern >= 0) {
        textpos_y = 200;
        textpos_x = 10;
        minifont_draw_str(vram_s + (640 * textpos_y) + textpos_x, 640,
                          "Loaded baked pattern:");
        minifont_set_color(0, 255, 0); /* Green */
        textpos_y += 16;
        minifont_draw_str(vram_s + (640 * textpos_y) + textpos_x + 20, 640,
                          catalog[loaded_pattern].description);
    }

    /* Draw the bottom half of the screen and finish it up. */
    textpos_y = 360;
    textpos_x = 10;
    minifont_set_color(255, 255, 255); /* White */
    const char *instructions[] = {"Press left/right to switch field.",
                                  "Press up/down to change values.",
                                  "Press A to send effect to rumblepack.",
                                  "Press B to stop rumble.",
                                  "Press X for next baked pattern",
                                  "Press Start to quit."
                                 };

    for (size_t i = 0; i < sizeof(instructions) / sizeof(instructions[0]);
         i++) {
        minifont_draw_str(vram_s + (640 * textpos_y) + textpos_x, 640,
                          instructions[i]);
        textpos_y += 16;
    }

    vid_flip(-1);
}

/* This blocks waiting for a specified device to be present and valid */
void wait_for_dev_attach(maple_device_t **dev_ptr, unsigned int func) {
    maple_device_t *dev = NULL;

    do {
        /* If we already have it, and it's still valid, leave */
        for(int u = 0; u < MAPLE_UNIT_COUNT; u++) {
            /* Only check for valid device on port A/0 */
            dev = maple_enum_dev(0, u);

            if(dev != NULL && dev->valid && (dev->info.functions & func)) {
                *dev_ptr = dev;
                return;
            }
        }
    }
    while((dev == NULL) || !dev->valid);

    /* Draw up a screen */
    vid_clear(0, 0, 0);
    int textpos_x = 40, textpos_y = 200;

    switch (func) {
        case MAPLE_FUNC_CONTROLLER:
            minifont_draw_str(vram_s + (640 * textpos_y) + textpos_x, 640,
                              "Please attach a controller to port A!");
            break;

        case MAPLE_FUNC_PURUPURU:
            minifont_draw_str(
                vram_s + (640 * textpos_y) + textpos_x, 640,
                "Please attach a rumbler to controller in port A!");

        default:
            break;
    }

    vid_flip(-1);
}

int main(int argc, char *argv[]) {
    cont_state_t *state;
    maple_device_t *contdev = NULL, *purudev = NULL;

    uint16_t old_buttons = 0, rel_buttons = 0;

    vid_set_mode(DM_640x480 | DM_MULTIBUFFER, PM_RGB565);

    /* Loop until Start is pressed */
    while (!(rel_buttons & CONT_START)) {
        /* Before drawing the screen, trap into these functions to be
           sure that there's at least one controller and one rumbler */
        wait_for_dev_attach(&contdev, MAPLE_FUNC_CONTROLLER);
        wait_for_dev_attach(&purudev, MAPLE_FUNC_PURUPURU);

        /* Store current button states + buttons which have been released. */
        state = (cont_state_t *)maple_dev_status(contdev);

        /* Make sure we can rely on the state, otherwise loop. */
        if (state == NULL) continue;

        rel_buttons = (old_buttons ^ state->buttons);

        if ((state->buttons & CONT_DPAD_LEFT) &&
            (rel_buttons & CONT_DPAD_LEFT)) {
            cursor_pos = cursor_pos - 1;

            if (cursor_pos < 0) cursor_pos = num_fields - 1;

            if (cursor_pos == 1) cursor_pos = 0;
        }

        if ((state->buttons & CONT_DPAD_RIGHT) &&
            (rel_buttons & CONT_DPAD_RIGHT)) {
            cursor_pos = (cursor_pos + 1) % num_fields;

            if (cursor_pos == 1) cursor_pos = 2;
        }

        int delta = (state->buttons & CONT_DPAD_UP)
                    ? 1
                    : ((state->buttons & CONT_DPAD_DOWN) ? -1 : 0);

        if (delta) {
            alter_field_at_offset(cursor_pos, delta);
            loaded_pattern = -1;
            usleep(100000); /* 1/10th second to make it humanely manageable*/
        }

        if ((state->buttons & CONT_X) && (rel_buttons & CONT_X)) {
            effect = catalog[catalog_index].effect;
            loaded_pattern = catalog_index;
            catalog_index++;

            if (catalog_index >= sizeof(catalog) / sizeof(baked_pattern_t))
                catalog_index = 0;
        }


        if ((state->buttons & CONT_A) && (rel_buttons & CONT_A)) {
            /* We print these out to make it easier to track the options chosen
             */
            printf("Rumble effect hex code: 0x%lx!\n", effect.raw);
            purupuru_rumble(purudev, &effect);
        }

        if ((state->buttons & CONT_B) && (rel_buttons & CONT_B)) {
            purupuru_rumble(purudev, &rumble_stop);
            printf("Rumble Stopped!\n");
        }

        old_buttons = state->buttons;
        redraw_screen();
    }

    /* Stop rumbling before exiting, if it still exists. */
    if ((purudev != NULL) && purudev->valid)
        purupuru_rumble(purudev, &rumble_stop);

    return 0;
}
