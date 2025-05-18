// Originally from plib_examples
// Ported to Dreamcast/KOS by Peter Hatch
// read_input () code from KOS examples
// Converted to a clock by Megan Potter
// Resolution enhanced by Falco Girgis

#include <kos.h>
#include <time.h>
#include <dcplib/fnt.h>

fntRenderer *text;
fntTexFont *font;

int filter_mode = 0;

const char *days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
                         "Aug", "Sep", "Oct", "Nov", "Dec"
                       };

float bg[3];                        /* Current bg */
float bg_delta[3] = { 0.01f };      /* bg per-frame delta */
int   bg_cur = 1;
float bgarray[][3] = {
    { 0.0f, 0.0f, 0.0f },
    { 0.5f, 0.0f, 0.0f },
    { 0.0f, 0.5f, 0.0f },
    { 0.0f, 0.0f, 0.5f },
    { 0.5f, 0.0f, 0.5f },
    { 0.0f, 0.5f, 0.5f },
    { 0.5f, 0.5f, 0.0f },
    { 0.5f, 0.5f, 0.5f }
};
#define BG_COUNT 8

#define fabs(a) ( (a) < 0 ? -(a) : (a) )
void bgframe() {
    pvr_set_bg_color(bg[0], bg[1], bg[2]);

    bg[0] += bg_delta[0];
    bg[1] += bg_delta[1];
    bg[2] += bg_delta[2];

    if(fabs(bg[0] - bgarray[bg_cur][0]) < 0.01f
            && fabs(bg[1] - bgarray[bg_cur][1]) < 0.01f
            && fabs(bg[2] - bgarray[bg_cur][2]) < 0.01f) {
        bg_cur++;

        if(bg_cur >= BG_COUNT)
            bg_cur = 0;

        bg_delta[0] = (bgarray[bg_cur][0] - bg[0]) / (0.5f / 0.01f);
        bg_delta[1] = (bgarray[bg_cur][1] - bg[1]) / (0.5f / 0.01f);
        bg_delta[2] = (bgarray[bg_cur][2] - bg[2]) / (0.5f / 0.01f);
    }
}

void drawFrame() {
    struct timespec spec;
    struct tm tm;
    char tmpbuf[256];
    int y = 50;

    bgframe();

    timespec_get(&spec, TIME_UTC);
    localtime_r(&spec.tv_sec, &tm);

    pvr_wait_ready();
    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_TR_POLY);

    text->setFilterMode(filter_mode);

    text->setFont(font);
    text->setPointSize(30);

    text->begin();
    text->setColor(1, 1, 1);
    text->start2f(20, y);
    text->puts("(High Res) Simple DC Clock");
    text->end();
    y += 50;

    sprintf(tmpbuf, "Unix: %lld", spec.tv_sec);

    text->begin();
    text->setColor(1, 1, 1);
    text->start2f(20, y);
    text->puts(tmpbuf);
    text->end();
    y += 50;

    sprintf(tmpbuf, "%s %s %02d %04d",
            days[tm.tm_wday], months[tm.tm_mon], tm.tm_mday, 1900 + tm.tm_year);

    text->begin();
    text->setColor(1, 1, 1);
    text->start2f(20, y);
    text->puts(tmpbuf);
    text->end();
    y += 50;

    sprintf(tmpbuf, "%02d:%02d:%02d.%lu",
            tm.tm_hour, tm.tm_min, tm.tm_sec, spec.tv_nsec);

    text->begin();
    text->setColor(1, 1, 1);
    text->start2f(20, y);
    text->puts(tmpbuf);
    text->end();
    y += 50;

    pvr_list_finish();
    pvr_scene_finish();
}


/* This is really more complicated than it needs to be in this particular
   case, but it's nice and verbose. */
int read_input() {
    maple_device_t *cont;
    cont_state_t *state;

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

    return 0;
}

int main(int argc, char **argv) {
    pvr_init_params_t pvrInit = {
        {PVR_BINSIZE_0, PVR_BINSIZE_0, PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_0},
        512 * 1024, 0, 0, 0, 0, 0
    };

    pvr_init(&pvrInit);

    text = new fntRenderer();
    font = new fntTexFont("/rd/default.txf");

    while(!read_input()) {
        drawFrame();
    }

    return 0;
}
