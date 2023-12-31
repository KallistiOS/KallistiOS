/*  KallistiOS ##version##

    beep.c
    Copyright (C) 2004 SinisterTengu
    Copyright (C) 2008 Donald Haase

*/

/*
    This example allows you to send raw commands to the VMU's buzzer, using the CLOCK
    function implemented by the VMU.

    On a typical VMU, which only has a single channel, only the low 2 bytes are used to
    create a single waveform, with the lowest byte being the period of the waveform, and
    the next byte being the duty cycle. Duty cycle should always be less than the period
    and is recommended to stay around 50%.

    All of the interface code for this was stolen from SinisterTengu's Puru Puru Demo from '04
    (when Kamjin first RE'd the puru). The rest was based off the puru driver as well as other bits
    of maple driver and lots of guesswork (and messed up vmu).

    Try out this value: 0x000065F0

    Enjoy, and let me know about some interesting sounds.
 */

#include <stdio.h>
#include <stdint.h>

#include <kos/init.h>

#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/vmu.h>

#include <arch/arch.h>

#include <plx/font.h>

KOS_INIT_FLAGS(INIT_DEFAULT);

int main(int argc, char *argv[]) {
    maple_device_t *dev, *vmudev;
    cont_state_t *state;
    plx_font_t *fnt;
    plx_fcxt_t *cxt;
    point_t w;
    int i = 0, count = 0;
    uint16_t old_buttons = 0, rel_buttons = 0;
    uint32_t effect = 0;
    uint8_t n[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; //nibbles
    char s[8][2] = { "", "", "", "", "", "", "", "" };

    /* If the face buttons are all pressed, exit the app */
    cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
                      (cont_btn_callback_t)arch_exit);

    pvr_init_defaults();

    fnt = plx_font_load("/rd/axaxax.txf");
    cxt = plx_fcxt_create(fnt, PVR_LIST_TR_POLY);

    pvr_set_bg_color(0.0f, 0.0f, 0.0f);

    for(;;) {
        dev = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
        vmudev = maple_enum_type(0, MAPLE_FUNC_CLOCK);

        while(!dev) {
            pvr_wait_ready();
            pvr_scene_begin();
            pvr_list_begin(PVR_LIST_OP_POLY);
            pvr_list_begin(PVR_LIST_TR_POLY);

            plx_fcxt_begin(cxt);
            w.x = 40.0f; w.y = 200.0f; w.z = 10.0f;
            plx_fcxt_setpos_pnt(cxt, &w);
            plx_fcxt_draw(cxt, "Please attach a controller!");
            plx_fcxt_end(cxt);

            pvr_scene_finish();

            dev = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
        }

        while(!vmudev) {
            pvr_wait_ready();
            pvr_scene_begin();
            pvr_list_begin(PVR_LIST_OP_POLY);
            pvr_list_begin(PVR_LIST_TR_POLY);

            plx_fcxt_begin(cxt);
            w.x = 40.0f; w.y = 200.0f; w.z = 10.0f;
            plx_fcxt_setpos_pnt(cxt, &w);
            plx_fcxt_draw(cxt, "Please attach a vmu!");
            plx_fcxt_end(cxt);

            pvr_scene_finish();

            vmudev = maple_enum_type(0, MAPLE_FUNC_CLOCK);
        }

        pvr_wait_ready();
        pvr_scene_begin();
        pvr_list_begin(PVR_LIST_OP_POLY);
        pvr_list_begin(PVR_LIST_TR_POLY);
        plx_fcxt_begin(cxt);

        w.x = 70.0f; w.y = 70.0f; w.z = 10.0f;
        plx_fcxt_setpos_pnt(cxt, &w);
        plx_fcxt_draw(cxt, "VMUBeep Test by Quzar");

        w.x += 130; w.y += 120.0f;
        plx_fcxt_setpos_pnt(cxt, &w);
        plx_fcxt_setsize(cxt, 30.0f);
        plx_fcxt_draw(cxt, "0x");

        w.x += 48.0f;
        plx_fcxt_setpos_pnt(cxt, &w);

        count = 0;
        while(count <= 7) {
            if(i == count)
                plx_fcxt_setcolor4f(cxt, 1.0f, 0.9f, 0.9f, 0.0f);
            else
                plx_fcxt_setcolor4f(cxt, 1.0f, 1.0f, 1.0f, 1.0f);

            sprintf(s[count], "%x", n[count]);
            plx_fcxt_draw(cxt, s[count]);

            count++;
            w.x += 25.0f;
        }

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
        plx_fcxt_draw(cxt, "Press A to start vmu beep.");
        w.y += 25.0f;

        plx_fcxt_setpos_pnt(cxt, &w);
        plx_fcxt_draw(cxt, "Press B to stop vmu beep.");
        w.y += 25.0f;

        plx_fcxt_setpos_pnt(cxt, &w);
        plx_fcxt_draw(cxt, "Press ABXYS to quit.");

        plx_fcxt_end(cxt);
        pvr_scene_finish();

        /* Store current button states + buttons which have been released. */
        state = (cont_state_t *)maple_dev_status(dev);
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

            vmu_beep_raw(vmudev, effect);
            printf("VMU Beep: 0x%lx!\n", effect);
        }

        if((state->buttons & CONT_B) && (rel_buttons & CONT_B)) {
            vmu_beep_raw(vmudev, 0x00000000);
            printf("Beep Stopped!\n");
        }

        old_buttons = state->buttons ;
    }

    return 0;
}
