/*  KallistiOS ##version##

    rumble.c
    Copyright (C) 2004 SinisterTengu
    Copyright (C) 2008, 2023, 2025 Donald Haase
    Copyright (C) 2024 Daniel Fairchild

*/

/*
    This example allows you to send raw commands to the rumble accessory (aka purupuru).

    This is a recreation of an original posted by SinisterTengu in 2004 here:
    https://dcemulation.org/phpBB/viewtopic.php?p=490067#p490067 . Unfortunately,
    that one is lost, but I had based my vmu_beep testing on it, and the principle is
    the same. In each, a single 32-bit value is sent to the device which defines the
    features of the rumbling.

    TODO: This should be updated at some point to display and work from the macros in
    dc/maple/purupuru.h that define the characteristics of the raw 32-bit value.

 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <kos/init.h>

#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/purupuru.h>

#include <plx/font.h>

KOS_INIT_FLAGS(INIT_DEFAULT);

plx_fcxt_t *cxt;

void print_rumble_fields(purupuru_effect_t fields) {

  printf("Rumble Fields:\n");
  printf("  .cont   =  %s,\n", fields.cont ? "true" : "false");
  printf("  .motor  =  %u,\n", fields.motor);

  printf("  .bpow   =  %u,\n", fields.bpow);
  printf("  .fpow   =  %u,\n", fields.fpow);
  printf("  .div    =  %s,\n", fields.div ? "true" : "false");
  printf("  .conv   =  %s,\n", fields.conv ? "true" : "false");

  printf("  .freq   =  %u,\n", fields.freq);
  printf("  .inc    =  %u,\n", fields.inc);
}
/* This blocks waiting for a specified device to be present and valid */
void wait_for_dev_attach(maple_device_t **dev_ptr, unsigned int func) {
    maple_device_t *dev = *dev_ptr;
    point_t w = {40.0f, 200.0f, 10.0f, 0.0f};

    /* If we already have it, and it's still valid, leave */
    /* dev->valid is set to false by the driver if the device
       is detached, but dev will stay not-null */
    if((dev != NULL) && dev->valid)
        return;

    /* Draw up a screen */
    pvr_wait_ready();
    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_OP_POLY);
    pvr_list_begin(PVR_LIST_TR_POLY);

    plx_fcxt_begin(cxt);
    plx_fcxt_setpos_pnt(cxt, &w);
    if(func == MAPLE_FUNC_CONTROLLER)
        plx_fcxt_draw(cxt, "Please attach a controller!");
    else if(func == MAPLE_FUNC_PURUPURU)
        plx_fcxt_draw(cxt, "Please attach a rumbler!");
    plx_fcxt_end(cxt);

    pvr_scene_finish();

    /* Repeatedly check until we find one and it's valid */
    while((dev == NULL) || !dev->valid) {
        *dev_ptr = maple_enum_type(0, func);
        dev = *dev_ptr;
        usleep(50);
    }
}


typedef struct {
  purupuru_effect_t effect;
  const char *description;
} baked_pattern_t;

/* motor cannot be 0 (will generate error on official hardware), but we can set everything else to 0 for stopping */
static const purupuru_effect_t rumble_stop = {.motor = 1};

static size_t catalog_index = 0;
static const baked_pattern_t catalog[] = {
    {.effect = {.cont = false, .motor = 1, .fpow = 7, .freq = 26, .inc =  1}, .description = "Basic Thud (simple .5s jolt)"},
    {.effect = {.cont = true,  .motor = 1, .fpow = 1, .freq =  7, .inc = 49}, .description = "Car Idle (69 Mustang)"},
    {.effect = {.cont = false, .motor = 1, .fpow = 7, .conv = true, .freq = 21, .inc = 38}, .description = "Car Idle (VW beetle)"},
    {.effect = {.cont = false, .motor = 1, .fpow = 7, .conv = true, .freq = 57, .inc = 51}, .description = "Eathquake (Vibrate, and fade out)"},
    {.effect = {.cont = true,  .motor = 1, .fpow = 1, .freq =  40, .inc = 5}, .description = "Helicopter"},
    {.effect = {.cont = false, .motor = 1, .fpow = 2, .freq =  7, .inc = 0}, .description = "Ship's Thrust (as in AAC)"},
};

static inline void word2hexbytes(uint32_t word, uint8_t *bytes) {
  for (int i = 0; i < 8; i++) {
    bytes[i] = (word >> (28 - (i * 4))) & 0xf;
  }
}

int main(int argc, char *argv[]) {

    cont_state_t *state;
    maple_device_t *contdev = NULL, *purudev = NULL;

    plx_font_t *fnt;
    point_t w;
    int i = 0, count = 0;
    uint16_t old_buttons = 0, rel_buttons = 0;
    purupuru_effect_t effect = {.raw = 0};
    uint8_t n[8];
    char s[8][2] = { "", "", "", "", "", "", "", "" };
    word2hexbytes(0, n);

    pvr_init_defaults();

    fnt = plx_font_load("/rd/axaxax.txf");
    cxt = plx_fcxt_create(fnt, PVR_LIST_TR_POLY);

    pvr_set_bg_color(0.0f, 0.0f, 0.0f);

    /* Loop until Start is pressed */
    while(!(rel_buttons & CONT_START)) {

        /* Before drawing the screen, trap into these functions to be
           sure that there's at least one controller and one rumbler */
        wait_for_dev_attach(&contdev, MAPLE_FUNC_CONTROLLER);
        wait_for_dev_attach(&purudev, MAPLE_FUNC_PURUPURU);

        /* Start drawing and draw the header */
        pvr_wait_ready();
        pvr_scene_begin();
        pvr_list_begin(PVR_LIST_OP_POLY);
        pvr_list_begin(PVR_LIST_TR_POLY);
        plx_fcxt_begin(cxt);

        w.x = 70.0f; w.y = 70.0f; w.z = 10.0f;
        plx_fcxt_setpos_pnt(cxt, &w);
        plx_fcxt_draw(cxt, "Rumble Test by Quzar");

        /* Start drawing the changeable section of the screen */
        w.x += 130; w.y += 120.0f;
        plx_fcxt_setpos_pnt(cxt, &w);
        plx_fcxt_setsize(cxt, 30.0f);
        plx_fcxt_draw(cxt, "0x");

        w.x += 48.0f;
        plx_fcxt_setpos_pnt(cxt, &w);

        for(count = 0; count <= 7; count++, w.x += 25.0f) {
            if(i == count)
                plx_fcxt_setcolor4f(cxt, 1.0f, 0.9f, 0.9f, 0.0f);
            else
                plx_fcxt_setcolor4f(cxt, 1.0f, 1.0f, 1.0f, 1.0f);

            sprintf(s[count], "%x", n[count]);

            plx_fcxt_draw(cxt, s[count]);
        }

        /* Store current button states + buttons which have been released. */
        state = (cont_state_t *)maple_dev_status(contdev);

        /* Make sure we can rely on the state, otherwise loop. */
        if(state == NULL)
            continue;

        rel_buttons = (old_buttons ^ state->buttons);

        if((state->buttons & CONT_DPAD_LEFT) && (rel_buttons & CONT_DPAD_LEFT)) {
            if(i > 0) i--;
        }

        if((state->buttons & CONT_DPAD_RIGHT) && (rel_buttons & CONT_DPAD_RIGHT)) {
            if(i < 7) i++;
        }

        if((state->buttons & CONT_DPAD_UP) && (rel_buttons & CONT_DPAD_UP)) {
            if(n[i] < 15) n[i]++;
        }

        if((state->buttons & CONT_DPAD_DOWN) && (rel_buttons & CONT_DPAD_DOWN)) {
            if(n[i] > 0) n[i]--;
        }

        if ((state->buttons & CONT_X) && (rel_buttons & CONT_X)) {
            printf("Setting baked effect:\n\t'%s'\n", catalog[catalog_index].description);
            word2hexbytes(catalog[catalog_index].effect.raw, n);
            catalog_index++;
            if (catalog_index >= sizeof(catalog) / sizeof(baked_pattern_t)) {
                catalog_index = 0;
            }
        }


        if((state->buttons & CONT_A) && (rel_buttons & CONT_A)) {
            effect.raw = (n[0] << 28) + (n[1] << 24) + (n[2] << 20) + (n[3] << 16) +
                     (n[4] << 12) + (n[5] << 8) + (n[6] << 4) + (n[7] << 0);

            purupuru_rumble(purudev, &effect);
            /* We print these out to make it easier to track the options chosen */
            printf("Rumble: 0x%lx!\n", effect.raw);
            print_rumble_fields(effect);
        }

        if((state->buttons & CONT_B) && (rel_buttons & CONT_B)) {
            purupuru_rumble(purudev, &rumble_stop);
            printf("Rumble Stopped!\n");
        }

        old_buttons = state->buttons ;

        /* Draw the bottom half of the screen and finish it up. */
        plx_fcxt_setsize(cxt, 24.0f);
        plx_fcxt_setcolor4f(cxt, 1.0f, 1.0f, 1.0f, 1.0f);
        w.x = 65.0f; w.y += 50.0f;

        plx_fcxt_setpos_pnt(cxt, &w);
        plx_fcxt_draw(cxt, "Press left/right to switch digits.");
        w.y += 25.0f;

        plx_fcxt_setpos_pnt(cxt, &w);
        plx_fcxt_draw(cxt, "Press up/down to change values.");
        w.y += 25.0f;

        plx_fcxt_setpos_pnt(cxt, &w);
        plx_fcxt_draw(cxt, "Press A to start rumblin.");
        w.y += 25.0f;

        plx_fcxt_setpos_pnt(cxt, &w);
        plx_fcxt_draw(cxt, "Press B to stop rumblin.");
        w.y += 25.0f;

        plx_fcxt_setpos_pnt(cxt, &w);
        plx_fcxt_draw(cxt, "Press X for next baked pattern");
        w.y += 25.0f;

        plx_fcxt_setpos_pnt(cxt, &w);
        plx_fcxt_draw(cxt, "Press Start to quit.");

        plx_fcxt_end(cxt);
        pvr_scene_finish();
    }

    /* Stop rumbling before exiting, if it still exists. */
    if((purudev != NULL) && purudev->valid)
        purupuru_rumble(purudev, &rumble_stop);

    plx_font_destroy(fnt);
    plx_fcxt_destroy(cxt);

    return 0;
}
