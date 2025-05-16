/* This is a port of my original 3dtest example to KOS. The big difference
   here is that the background plane is no longer used. */

#include <kos.h>

/* This is really more complicated than it needs to be in this particular
   case, but it's nice and verbose. */
int check_start(void) {
    maple_device_t *cont;
    cont_state_t *state;

    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    if(!cont)
        return 0;

    /* Check for start on the controller */
    state = (cont_state_t *)maple_dev_status(cont);

    if(!state) {
        return 0;
    }

    if(state->buttons & CONT_START) {
        printf("Pressed start\n");
        return 1;
    }

    return 0;
}


/* A little test program -- creates twelve rainbow polygons and
   moves them around over a color-shifting background. */
typedef struct {
    float   x, y, z;
    float   dx, dy;
} polyplace_t;

polyplace_t polys[12] = {
    { 16.0f, 0.0f, 10.0f, 3.0f, 5.0f }, /* Top */
    { 300.0f, 0.0f, 11.0f, 0.0f, 6.0f },
    { 624.0f, 0.0f, 12.0f, -3.0f, 7.0f },

    { 24.0f, 300.0f, 13.0f, 3.0f, -7.0f },  /* Bottom */
    { 340.0f, 300.0f, 14.0f, 0.0f, -6.0f },
    { 616.0f, 300.0f, 15.0f, -3.0f, -5.0f },

    { 0.0f, 16.0f, 16.0f, 5.0f, 3.0f }, /* Left */
    { 0.0f, 220.0f, 17.0f, 6.0f, 0.0f },
    { 0.0f, 364.0f, 18.0f, 7.0f, -3.0f },

    { 480.0f, 24.0f, 19.0f, -7.0f, 3.0f },  /* Right */
    { 480.0f, 260.0f, 20.0f, -6.0f, 0.0f },
    { 480.0f, 356.0f, 21.0f, -5.0f, -3.0f },
};

/* Does one frame of polygon movement */
void move_polys(void) {
    int i;
    polyplace_t *p;

    for(i = 0; i < 12; i++) {
        p = polys + i;

        p->x += p->dx;

        if(p->x < 0 || p->x > 580.0f) {
            p->dx = -p->dx;
            p->x += p->dx;
        }

        p->y += p->dy;

        if(p->y < 0 || p->y > 420.0f) {
            p->dy = -p->dy;
            p->y += p->dy;
        }
    }
}

/* Sends one polygon's worth of data to the TA */
void draw_one_poly(polyplace_t *p) {
    /* Opaque Colored vertex */
    pvr_vertex_t vert;

    vert.flags = PVR_CMD_VERTEX;
    vert.x = p->x + 0.0f;
    vert.y = p->y + 200.0f;
    vert.z = p->z;
    vert.u = vert.v = 0.0f;
    vert.argb = 0xff0000ff;
    vert.oargb = 0;
    pvr_prim(&vert, sizeof(vert));

    vert.y = p->y + 0.0f;
    vert.argb = 0xff00ff00;
    pvr_prim(&vert, sizeof(vert));

    vert.x = p->x + 200.0f;
    vert.y = p->y + 200.0f;
    vert.argb = 0xff000000;
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.y = p->y + 0.0f;
    vert.argb = 0xffff0000;
    pvr_prim(&vert, sizeof(vert));
}

uint32_t color = 0;
int dcolor = 5;
void draw_frame(void) {
    pvr_poly_cxt_t  cxt;
    pvr_poly_hdr_t  poly;
    int i;

    pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
    pvr_poly_compile(&poly, &cxt);

    color += dcolor;

    if(color <= 0) {
        color = 0;
        dcolor = -dcolor;
    }

    if(color >= 255) {
        color = 255;
        dcolor = -dcolor;
    }

    /* Start opaque poly list */
    pvr_wait_ready();
    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_OP_POLY);

    /* Send polygon header to the TA using store queues */
    pvr_prim(&poly, sizeof(poly));

    /* Draw all polygons */
    for(i = 0; i < 12; i++)
        draw_one_poly(polys + i);

    /* End of opaque list */
    pvr_list_finish();

    /* Finish the frame */
    pvr_scene_finish();

    /* Move all polygons */
    move_polys();
}

/* Main program: init and loop drawing polygons */
pvr_init_params_t pvr_params = {
    { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0, PVR_BINSIZE_0, PVR_BINSIZE_0 },
    64 * 1024, 0, 0, 0, 0, 0
};
int main(int argc, char **argv) {
    pvr_init(&pvr_params);

    pvr_set_bg_color(1.0f, 1.0f, 1.0f);

    while(!check_start())
        draw_frame();

    /* Shutdown isn't technically necessary, but is possible */
    pvr_shutdown();

    return 0;
}

