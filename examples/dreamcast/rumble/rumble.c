/*  KallistiOS ##version##

    rumble.c
    Copyright (C) 2004 SinisterTengu
    Copyright (C) 2008, 2023 Donald Haase

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

#include <kos/init.h>

#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/purupuru.h>

#include <plx/font.h>

KOS_INIT_FLAGS(INIT_DEFAULT);

plx_fcxt_t *cxt;

/* This blocks waiting for a specified device to be present and valid */
void wait_for_dev_attach(maple_device_t **dev_ptr, unsigned int func) {
    maple_device_t *dev = *dev_ptr;
    point_t w = {40.0f, 200.0f, 10.0f, 0.0f};

    /* If we already have it, and it's still valid, leave */
    /* dev->valid is set to 0 by the driver if the device
       is detached, but dev will stay not-null */
    if((dev != NULL) && (dev->valid != 0)) return;

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
    while((dev == NULL) || (dev->valid == 0)) {
        *dev_ptr = maple_enum_type(0, func);
        dev = *dev_ptr;
        usleep(50);
    }
}

int main(int argc, char *argv[]) {

    cont_state_t *state;
    maple_device_t *contdev = NULL, *purudev = NULL;

    plx_font_t *fnt;
    point_t w;
    int i = 0, count = 0;
    uint16_t old_buttons = 0, rel_buttons = 0;
    uint32_t effect = 0;
    uint8_t n[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; //nibbles
    char s[8][2] = { "", "", "", "", "", "", "", "" };

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

        if((state->buttons & CONT_A) && (rel_buttons & CONT_A)) {
            effect = (n[0] << 28) + (n[1] << 24) + (n[2] << 20) + (n[3] << 16) +
                     (n[4] << 12) + (n[5] << 8) + (n[6] << 4) + (n[7] << 0);

            purupuru_rumble_raw(purudev, effect);
            /* We print these out to make it easier to track the options chosen */
            printf("Rumble: 0x%lx!\n", effect);
        }

        if((state->buttons & CONT_B) && (rel_buttons & CONT_B)) {
            purupuru_rumble_raw(purudev, 0x00000000);
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
        plx_fcxt_draw(cxt, "Press Start to quit.");

        plx_fcxt_end(cxt);
        pvr_scene_finish();
    }

    /* Stop rumbling before exiting, if it still exists. */
    if((purudev != NULL) && (purudev->valid != 0))
        purupuru_rumble_raw(purudev, 0x00000000);

    plx_font_destroy(fnt);
    plx_fcxt_destroy(cxt);

    return 0;
}
