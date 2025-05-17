/* KallistiOS ##version##

   examples/dreamcast/pvr/modifier_volume/modifier.c
   Copyright (C) 2009 Lawrence Sebald

   This example shows off how to do a basic modifier volume.
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <dc/pvr.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

#define NUM_POLYS 10

static pvr_vertex_pcm_t verts[NUM_POLYS * 4];

static pvr_poly_mod_hdr_t phdr;
static pvr_mod_hdr_t mhdr, mhdr2;
static float mx = 320.0f, my = 240.0f;
static pvr_list_t list = PVR_LIST_OP_POLY;

void setup(void) {
    pvr_poly_cxt_t cxt;
    int i;
    float x, y, z;
    uint32 argb = list == PVR_LIST_OP_POLY ? 0xFF0000FF : 0x80FF00FF;

    pvr_poly_cxt_col_mod(&cxt, list);
    pvr_poly_mod_compile(&phdr, &cxt);

    pvr_mod_compile(&mhdr, list + 1, PVR_MODIFIER_OTHER_POLY, PVR_CULLING_NONE);
    pvr_mod_compile(&mhdr2, list + 1, PVR_MODIFIER_INCLUDE_LAST_POLY,
                    PVR_CULLING_NONE);

    for(i = 0; i < NUM_POLYS; ++i) {
        x = rand() % 640;
        y = rand() % 480;
        z = rand() % 100 + 1;

        verts[i * 4].flags = PVR_CMD_VERTEX;
        verts[i * 4].x = x - 50;
        verts[i * 4].y = y + 50;
        verts[i * 4].z = z;
        verts[i * 4].argb0 = argb;
        verts[i * 4].argb1 = 0xFF00FF00;
        verts[i * 4].d1 = verts[i * 4].d2 = 0;

        verts[i * 4 + 1].flags = PVR_CMD_VERTEX;
        verts[i * 4 + 1].x = x - 50;
        verts[i * 4 + 1].y = y - 50;
        verts[i * 4 + 1].z = z;
        verts[i * 4 + 1].argb0 = argb;
        verts[i * 4 + 1].argb1 = 0xFF00FF00;
        verts[i * 4 + 1].d1 = verts[i * 4 + 1].d2 = 0;

        verts[i * 4 + 2].flags = PVR_CMD_VERTEX;
        verts[i * 4 + 2].x = x + 50;
        verts[i * 4 + 2].y = y + 50;
        verts[i * 4 + 2].z = z;
        verts[i * 4 + 2].argb0 = argb;
        verts[i * 4 + 2].argb1 = 0xFF00FF00;
        verts[i * 4 + 2].d1 = verts[i * 4 + 2].d2 = 0;

        verts[i * 4 + 3].flags = PVR_CMD_VERTEX_EOL;
        verts[i * 4 + 3].x = x + 50;
        verts[i * 4 + 3].y = y - 50;
        verts[i * 4 + 3].z = z;
        verts[i * 4 + 3].argb0 = argb;
        verts[i * 4 + 3].argb1 = 0xFF00FF00;
        verts[i * 4 + 3].d1 = verts[i * 4 + 3].d2 = 0;
    }
}

int check_start(void) {
    maple_device_t *cont;
    cont_state_t *state;
    static int taken = 0;

    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    if(cont) {
        state = (cont_state_t *)maple_dev_status(cont);

        if(!state)
            return 0;

        if(state->buttons & CONT_START)
            return 1;

        if(state->buttons & CONT_DPAD_UP)
            my -= 1.0f;

        if(state->buttons & CONT_DPAD_DOWN)
            my += 1.0f;

        if(state->buttons & CONT_DPAD_LEFT)
            mx -= 1.0f;

        if(state->buttons & CONT_DPAD_RIGHT)
            mx += 1.0f;

        if((state->buttons & CONT_A) && !taken) {
            list = list == PVR_LIST_OP_POLY ? PVR_LIST_TR_POLY :
                   PVR_LIST_OP_POLY;
            setup();
            taken = 1;
        }
        else if(!(state->buttons & CONT_A)) {
            taken = 0;
        }
    }

    return 0;
}

void do_frame(void) {
    pvr_modifier_vol_t mod;
    int i;

    pvr_wait_ready();
    pvr_scene_begin();
    pvr_list_begin(list);

    pvr_prim(&phdr, sizeof(phdr));

    for(i = 0; i < NUM_POLYS; ++i) {
        pvr_prim(&verts[i * 4], sizeof(verts[i * 4]));
        pvr_prim(&verts[i * 4 + 1], sizeof(verts[i * 4 + 1]));
        pvr_prim(&verts[i * 4 + 2], sizeof(verts[i * 4 + 2]));
        pvr_prim(&verts[i * 4 + 3], sizeof(verts[i * 4 + 3]));
    }

    pvr_list_finish();

    pvr_list_begin(list + 1);

    pvr_prim(&mhdr, sizeof(mhdr));

    mod.flags = PVR_CMD_VERTEX_EOL;
    mod.ax = mx;
    mod.ay = my + 50.0f;
    mod.az = 150.0f;
    mod.bx = mx;
    mod.by = my;
    mod.bz = 150.0f;
    mod.cx = mx + 50.0f;
    mod.cy = my + 50.0f;
    mod.cz = 150.0f;
    mod.d1 = mod.d2 = mod.d3 = mod.d4 = mod.d5 = mod.d6 = 0;
    pvr_prim(&mod, sizeof(mod));

    pvr_prim(&mhdr2, sizeof(mhdr2));

    mod.flags = PVR_CMD_VERTEX_EOL;
    mod.ax = mx;
    mod.ay = my;
    mod.az = 150.0f;
    mod.bx = mx + 50.0f;
    mod.by = my + 50.0f;
    mod.bz = 150.0f;
    mod.cx = mx + 50.0f;
    mod.cy = my;
    mod.cz = 150.0f;
    mod.d1 = mod.d2 = mod.d3 = mod.d4 = mod.d5 = mod.d6 = 0;
    pvr_prim(&mod, sizeof(mod));

    pvr_list_finish();
    pvr_scene_finish();
}

static pvr_init_params_t pvr_params = {
    /* Enable Opaque, Opaque modifiers, Translucent, and Translucent
    modifiers. */
    {
        PVR_BINSIZE_16, PVR_BINSIZE_16, PVR_BINSIZE_16, PVR_BINSIZE_16,
        PVR_BINSIZE_0
    },
    512 * 1024, 0, 0, 0, 0, 0
};

int main(int argc, char *argv[]) {
    printf("---KallistiOS PVR Modifier Example---\n");
    printf("Press A to toggle between translucent and opaque polygons.\n");
    printf("Use the DPAD to move the modifier square around (it starts at ");
    printf("(320, 240))\n");
    printf("Press Start to exit.\n");

    srand(time(NULL));

    pvr_init(&pvr_params);

    setup();

    /* Go as long as the user hasn't pressed start on controller 1. */
    while(!check_start())   {
        do_frame();
    }

    return 0;
}
