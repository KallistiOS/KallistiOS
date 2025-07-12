/* GhettoPlay: an S3M browser and playback util
   (c)2000 Megan Potter
*/

#include <oggvorbis/sndoggvorbis.h>
#include <string.h>
#include "gp.h"

/* Takes care of the song menu */


/* Song menu choices */
typedef struct {
    char    fn[256];
    int size;
} entry;
typedef struct {
    char    fn[256];
    int size;
} lst_entry;

///////// Mutex protected /////////
static mutex_t mut = MUTEX_INITIALIZER;

char curdir[1024] = "/";
char playdir[1024] = "/";
char loadme[256] = "";
char workstring[256] = "";

static entry entries[200];
static int num_entries = 0;
//////// Mutex protected //////////
static volatile int load_queued = 0;

static int selected = 0, top = 0, iplaying = 0;

static int framecnt = 0;
static float throb = 0.2f, dthrob = 0.01f;

/* Here we hold the playlist */
static int lst_size = 0;
static lst_entry lst_entries[200];
static int lst_playing = -1;


/* Code to draw a nice o-scope on the background during playback :) */
static mutex_t hookmut = MUTEX_INITIALIZER;
static int16_t sndbuffer[2][16384];
static int sndbuffer_cnt = 0;
static uint64_t last_time = 0;
static int takeframes = 0;
static int curpos = 0;
static int filtinitted = 0;

static void snd_hook(int strm, void * obj, int freq, int chn, void ** buf, int *req) {
    (void)strm; (void)obj, (void)freq, (void)chn;
    int actual;
    uint64_t t;

    mutex_lock(&hookmut);
    actual = *req;

    if(actual > 16384 * 2)
        actual = 16384 * 2;

    memcpy(sndbuffer + 0, sndbuffer + 1, sndbuffer_cnt * 4);
    memcpy(sndbuffer + 1, *buf, actual);
    sndbuffer_cnt = actual / 4;

    t = timer_ms_gettime64();

    if(last_time != 0) {
        uint32_t elapsed = (uint32_t)(t - last_time);
        takeframes = elapsed * 60 / 1000;
    }

    last_time = t;
    curpos = 0;
    mutex_unlock(&hookmut);
}

#include <plx/dr.h>
#include <plx/prim.h>
static void draw_wave(void) {
    int idx, cnt;
    float x, p, y, z = 80.0f;

    mutex_lock(&hookmut);

    if(takeframes != 0 && sndbuffer_cnt != 0) {
        plx_dr_state_t dr;
        plx_dr_init(&dr);
        cnt = sndbuffer_cnt * takeframes / 60;

        plx_vert_ind(&dr, PLX_VERT, 0.0f, 240.0f, z, 0xffffffff);
        plx_vert_ind(&dr, PLX_VERT, 0.0f, 235.0f, z, 0xffffffff);

        for(x = 20.0f, p = 0.0f; x < 620.0f; x += 4.0f, p += 4.0f / (620.0f - 20.0f)) {
            idx = curpos + ((int)(p * cnt)) * 2;
            y = 240.0f + sndbuffer[0][idx] * 120.0f / 32768.0f;
            plx_vert_ind(&dr, PLX_VERT, x, y + 5.0f, z, 0xffffffff);
            plx_vert_ind(&dr, PLX_VERT, x, y, z, 0xffffffff);
        }

        plx_vert_ind(&dr, PLX_VERT, 640.0f, 240.0f, z, 0xffffffff);
        plx_vert_ind(&dr, PLX_VERT_EOS, 640.0f, 235.0f, z, 0xffffffff);

        curpos += cnt;

        if(curpos >= (sndbuffer_cnt + sndbuffer_cnt * takeframes / 60))
            curpos = 0;
    }

    mutex_unlock(&hookmut);
}


static void *load_song_list(void * p) {
    (void)p;
    file_t d;

    d = fs_open(curdir, O_RDONLY | O_DIR);

    if(!d) {
        strcpy(curdir, "/");
        d = fs_open(curdir, O_RDONLY | O_DIR);

        if(!d) {
            mutex_lock(&mut);
            num_entries = 1;
            strcpy(entries[0].fn, "Error!");
            entries[0].size = 0;
            mutex_unlock(&mut);
            return NULL;
        }
    }

    {
        dirent_t *de;
        num_entries = 0;

        if(strcmp(curdir, "/")) {
            mutex_lock(&mut);
            strcpy(entries[0].fn, "<..>");
            entries[0].size = -1;
            num_entries++;
            mutex_unlock(&mut);
        }

        while((de = fs_readdir(d)) && num_entries < 200) {
            printf("read entry '%s'\n", de->name);

            if(strcmp(de->name, ".") && strcmp(de->name, "..")) {
                mutex_lock(&mut);
                strcpy(entries[num_entries].fn, de->name);
                entries[num_entries].size = de->size;
                num_entries++;
                mutex_unlock(&mut);
            }
        }
    }

    fs_close(d);
    load_queued = 0;
    return NULL;
}

/* Draws the song listing */
static void draw_listing(void) {
    float y = 92.0f;
    int i, esel;

//  printf("DEBUG: List size %d\r\n",lst_size);

    /* Draw all the song titles */
//  for (i=0; i<14 && (top+i)<num_entries; i++) {
    for(i = 0; i < 10 && (top + i) < num_entries; i++) {
        int li;
        draw_poly_strf(32.0f, y, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                       "%s", entries[top + i].fn);

        if(entries[top + i].size >= 0) {
            draw_poly_strf(32.0f + 240.0f, y, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                           "%d bytes", entries[top + i].size);
        }
        else {
            draw_poly_strf(32.0f + 240.0f, y, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f, "<DIR>");
        }

        /* Check for playlist entries */
        for(li = 0; li < lst_size; li++) {
            strcat(workstring, curdir);
            strcat(workstring, "/");
            strcat(workstring, entries[top + i].fn);

//          printf("%s =? %s \r\n",workstring,lst_entries[li].fn);

            if(!(strcmp(workstring, lst_entries[li].fn))) {
                draw_poly_strf(36.0f + 32.0f + 360.0f, y, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f, "%2d", li + 1);
            }

            if(!(strcmp(workstring, lst_entries[lst_playing].fn))) {
                draw_poly_box(31.0f, y - 1.0f,
                              609.0f, y + 25.0f, 90.0f,
                              throb, 0.0f, 0.2f, 0.2f,
                              throb, 0.0f, throb, 0.2f);

            }

            workstring[0] = 0;
        }


        y += 24.0f;
    }

    /* Put a highlight bar under one of them */
    esel = (selected - top);
    draw_poly_box(31.0f, 92.0f + esel * 24.0f - 1.0f,
                  609.0f, 92.0f + esel * 24.0f + 25.0f, 95.0f,
                  throb, throb, 0.2f, 0.2f, throb, throb, 0.2f, 0.2f);

    /* Put a highlight bar under the playing file */
    /* Only works correctly if you're in the same dir as the played file */
    if((sndoggvorbis_isplaying() && !(strcmp(playdir, curdir)) && (lst_size == 0))) {
        esel = (iplaying - top);
        draw_poly_box(31.0f, 92.0f + esel * 24.0f - 1.0f,
                      609.0f, 92.0f + esel * 24.0f + 25.0f, 90.0f,
                      throb, 0.0f, 0.2f, 0.2f,
                      throb, 0.0f, throb, 0.2f);
    }

    if(sndoggvorbis_isplaying()) {
        draw_wave();
    }

}

static void start(char *fn) {
    if(!sndoggvorbis_isplaying()) {
        sndoggvorbis_start(fn, 0);
    }
}

static void stop(void) {
    if(sndoggvorbis_isplaying()) {
        sndoggvorbis_stop();
    }
}

/* Handle controller input */
void check_controller(void) {
    static int up_moved = 0, down_moved = 0, a_pressed = 0, y_pressed = 0;
    maple_device_t *cont;
    cont_state_t *state;

    if((!(sndoggvorbis_isplaying())) && (lst_playing != -1)) {
        lst_playing++;

        if(lst_playing == lst_size) {
            printf("DEBUG: Reached end of playlist...\r\n");
            lst_playing = -1;
            lst_size = 0;
        }
        else {
            printf("DEBUG: Next song in playlist...\r\n");
            start(lst_entries[lst_playing].fn);
        }
    }

    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    if(!cont) {
        return;
    }

    state = (cont_state_t *)maple_dev_status(cont);

    if(!state) {
        return;
    }

    if(state->buttons & CONT_DPAD_UP) {
        if((framecnt - up_moved) > 10) {
            if(selected > 0) {
                selected--;

                if(selected < top) {
                    top = selected;
                }
            }

            up_moved = framecnt;
        }
    }

    if(state->buttons & CONT_DPAD_DOWN) {
        if((framecnt - down_moved) > 10) {
            if(selected < (num_entries - 1)) {
                selected++;

//              if (selected >= (top+14)) {
                if(selected >= (top + 10)) {
                    top++;
                }
            }

            down_moved = framecnt;
        }
    }

    if(state->ltrig > 0) {
        if((framecnt - up_moved) > 10) {
//          selected -= 14;
            selected -= 10;

            if(selected < 0) selected = 0;

            if(selected < top) top = selected;

            up_moved = framecnt;
        }
    }

    if(state->rtrig > 0) {
        if((framecnt - down_moved) > 10) {
//          selected += 14;
            selected += 10;

            if(selected > (num_entries - 1))
                selected = num_entries - 1;

//          if (selected >= (top+14))
            if(selected >= (top + 10))
                top = selected;

            down_moved = framecnt;
        }
    }

    if((state->buttons & CONT_B) && sndoggvorbis_isplaying()) {
        stop();
    }

    if(state->buttons & CONT_Y) {
        if((framecnt - y_pressed) > 10) {
            strcat(workstring, curdir);
            strcat(workstring, "/");
            strcat(workstring, entries[selected].fn);
            strcpy(lst_entries[lst_size].fn, workstring);

            printf("DEBUG: Entry %d : %s\r\n", lst_size, lst_entries[lst_size].fn);

            workstring[0] = 0;

            lst_size++;

        }

        y_pressed = framecnt;
    }

    if((state->buttons & CONT_A) && !load_queued) {
        if((framecnt - a_pressed) > 10) {
            if(!strcmp(entries[selected].fn, "Error!")) {
                num_entries = 0;
            }
            else if(lst_size > 0) {
                printf("DEBUG: Playing playlist...\r\n");
                lst_playing = 0;
                stop();
                start(lst_entries[lst_playing].fn);
                // sndoggvorbis_wait_play();
            }
            else if(entries[selected].size >= 0) {
                stop();
                strcpy(playdir, curdir);
                strcat(loadme, curdir);
                strcat(loadme, "/");
                strcat(loadme, entries[selected].fn);
                start(loadme);
                loadme[0] = 0;
                iplaying = selected;
            }
            else {
                if(!strcmp(entries[selected].fn, "<..>")) {
                    int i;

                    for(i = strlen(curdir); i > 0; i--) {
                        if(curdir[i] == '/') {
                            curdir[i] = 0;
                            break;
                        }
                        else {
                            curdir[i] = 0;
                        }
                    }
                }
                else {
                    if(strcmp(curdir, "/"))
                        strcat(curdir, "/");

                    strcat(curdir, entries[selected].fn);
                }

                mutex_lock(&mut);
                selected = top = num_entries = 0;
                mutex_unlock(&mut);
                printf("current directory is now '%s'\n", curdir);
            }
        }

        a_pressed = framecnt;
    }
}

/* Check maple bus inputs */
void check_inputs(void) {
    check_controller();
}

/* Main rendering of the song menu */
void song_menu_render(void) {

    if(!filtinitted) {
        snd_stream_filter_add(0, snd_hook, NULL);
        filtinitted = 1;
    }

    /* Draw a background box */
    draw_poly_box(30.0f, 80.0f, 610.0f, 440.0f - 96.0f, 90.0f,
                  0.2f, 0.8f, 0.5f, 0.0f, 0.2f, 0.8f, 0.8f, 0.2f);

    /* If we don't have a file listing, get it now */
    if(num_entries == 0 && !load_queued) {
        load_queued = 1;
        thd_create(1, load_song_list, NULL);
    }

    /* if (load_queued)
        draw_poly_strf(32.0f, 82.0f, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            "Scanning Directory..."); */

    /* Draw the song listing */
    mutex_lock(&mut);
    draw_listing();
    mutex_unlock(&mut);

    /* Adjust the throbber */
    throb += dthrob;

    if(throb < 0.2f || throb > 0.8f) {
        dthrob = -dthrob;
        throb += dthrob;
    }

    /* Check maple inputs */
    check_inputs();

    framecnt++;
}


