// Originally from plib_examples
// Ported to Dreamcast/KOS by Peter Hatch
// read_input () code from KOS examples

#include <kos.h>
#include <dcplib/fnt.h>

fntRenderer *text;

const int MAX_FONTS = 20;

const char *font_names[MAX_FONTS] = {
    "sorority.txf",          /* This is the default */
    "charter.txf",
    "courier-bold.txf",
    "courier_medium.txf",
    "curlfont.txf",
    "default.txf",
    "haeberli.txf",
    "helvetica_bold.txf",
    "helvetica_medium.txf",
    "lucida.txf",
    "lucidabright_bold.txf",
    "schoolbook_bold.txf",
    "schoolbook_medium.txf",
    "symbol.txf",
    "times_bold.txf",
    "times_medium.txf",
    "typewriter.txf",
    NULL
};

fntTexFont *font_list[MAX_FONTS];

int cur_font = 0;
int max_font = 0;

int filter_mode = 0;

void switchFont() {
    cur_font++;

    if(cur_font >= max_font)
        cur_font = 0;
}

void switchFilterMode() {
    filter_mode++;

    if(filter_mode >= 8)
        filter_mode = 0;
}

void drawFrame() {
    //ta_begin_render();
    pvr_wait_ready();
    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_TR_POLY);
    //pvr_dummy_poly (TA_OPAQUE);
    //ta_commit_eol ();

    text->setFilterMode(filter_mode);

    text->setFont(font_list[0]);
    text->setPointSize(25);

    text->begin();
    text->setColor(1, 1, 0);
    text->start2f(50, 50);
    text->puts("This is the PLIB Font Demo.");
    text->end();

    text->setFont(font_list[0]);
    text->setPointSize(12);

    text->begin();
    text->setColor(0.6f, 0.6f, 0.6f);
    text->start2f(320, 282);
    text->puts("The current font is:");
    text->start2f(350, 302);
    text->puts(font_names[cur_font]);
    text->start2f(320, 322);
    text->puts("Press A to change the font.\nPress Start to exit.");
    text->end();

    text->setFont(font_list[cur_font]);
    text->setPointSize(25);

    text->begin();
    text->setColor(0, 1, 1);
    text->start2f(50, 90);
    text->puts("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    text->setColor(1, 0, 1);
    text->start2f(50, 130);
    text->puts("abcdefghijklmnopqrstuvwxyz");
    text->setColor(1, 0, 0);
    text->start2f(50, 170);
    text->puts("0123456789!@#$%^&*()+-={}[]:\"'<>?,./`~_");
    text->end();

    /*
    glDisable ( GL_TEXTURE_2D ) ;
    glBegin ( GL_LINE_LOOP ) ;
    glColor3f ( 1, 0, 1 ) ;
     glVertex2f (   0,   0 ) ;
     glVertex2f ( 256,   0 ) ;
     glVertex2f ( 256, 256 ) ;
     glVertex2f (   0, 256 ) ;
    glEnd () ;
    */

    //vertex_ot_t vert;
    pvr_vertex_t vert;

    //vert.a = vert.r = vert.g = vert.b = 1.0f;
    vert.argb = 0xFFFFFFFF;
    //vert.oa = vert.or = vert.og = vert.ob = 0.0f;
    vert.oargb = 0;
    vert.z = 512.0f;

    //vert.flags = TA_VERTEX_NORMAL;
    vert.flags = PVR_CMD_VERTEX;
    vert.u = 0;
    vert.v = 0;
    vert.x = 50.0f;
    vert.y = 450.0f;
    //ta_commit_vertex (&vert, sizeof (vert));
    pvr_prim(&vert, sizeof(vert));

    //vert.flags = TA_VERTEX_NORMAL;
    vert.v = 1;
    vert.y = 194.0f;
    //ta_commit_vertex (&vert, sizeof (vert));
    pvr_prim(&vert, sizeof(vert));

    //vert.flags = TA_VERTEX_NORMAL;
    vert.u = 1;
    vert.v = 0;
    vert.x = 306.0f;
    vert.y = 450.0f;
    //ta_commit_vertex (&vert, sizeof (vert));
    pvr_prim(&vert, sizeof(vert));

    //vert.flags = TA_VERTEX_EOL;
    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.v = 1;
    vert.y = 194.0f;
    //ta_commit_vertex (&vert, sizeof (vert));
    pvr_prim(&vert, sizeof(vert));

    //ta_commit_eol ();
    pvr_list_finish();
    //ta_finish_frame ();
    pvr_scene_finish();
}

/* This is really more complicated than it needs to be in this particular
   case, but it's nice and verbose. */
int read_input() {
    maple_device_t *cont;
    cont_state_t *state;
    static bool aHeldDown = false;
    static bool bHeldDown = false;

    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    if(!cont) {
        return 0;
    }

    /* Check for start on the controller */
    state = (cont_state_t *)maple_dev_status(cont);

    if(!state) {
        printf("Error getting controller status\n");
        return 1;
    }

    if(state->buttons & CONT_START) {
        printf("Pressed start\n");
        return 1;
    }

    if(state->buttons & CONT_A) {
        if(!aHeldDown) {
            aHeldDown = true;
            switchFont();
        }
    }
    else {
        aHeldDown = false;
    }

    if(state->buttons & CONT_B) {
        if(!bHeldDown) {
            bHeldDown = true;
            switchFilterMode();
        }
    }
    else {
        bHeldDown = false;
    }

    return 0;
}

int main(int argc, char **argv) {
    pvr_init_params_t pvrInit = {
        {PVR_BINSIZE_0, PVR_BINSIZE_0, PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_0},
        512 * 1024, 0, 0, 0, 0, 0
    };

    pvr_init(&pvrInit);

    text = new fntRenderer();

    for(max_font = 0; font_names[max_font] != NULL; max_font++) {
        char fname[256];

        sprintf(fname, "/rd/%s", font_names[max_font]);

        font_list[max_font] = new fntTexFont(fname);
    }

    cur_font = 0;

    while(!read_input()) {
        drawFrame();
    }

    return 0;
}
