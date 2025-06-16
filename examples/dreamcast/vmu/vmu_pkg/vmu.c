/* KallistiOS ##version##

   vmu.c
   (c)2002 Megan Potter
*/

/* This simple example shows how to use the vmu_pkg_* functions to write
   a file to a VMU with a DC-compatible header so it can be managed like
   any other VMU file from the BIOS menus. */

#include <kos.h>

/* An icon is always 32x32 4bpp */
#define ICON_SIZE (32 * 32 / 2)

#define SCREEN_W 640
#define SCREEN_H 480

/* The Y indentation for the VMU Info text on screen */
#define INFO_Y 88

/* The amount of space from the top of one row of text to the next */
#define ROW_SPACER 24

#define NB_ICONS_MAX 3

void draw_dir(void) {
    file_t      d;
    int     y = INFO_Y;
    dirent_t    *de;

    d = fs_open("/vmu/a1", O_RDONLY | O_DIR);

    if(!d) {
        bfont_draw_str(vram_s + y * SCREEN_W + 10, SCREEN_W, 0, "Can't read VMU");
    }
    else {
        while((de = fs_readdir(d))) {
            bfont_draw_str(vram_s + y * SCREEN_W + 10, SCREEN_W, 0, de->name);
            y += ROW_SPACER;

            if(y >= (SCREEN_H - ROW_SPACER))
                break;
        }

        fs_close(d);
    }
}

int dev_checked = 0;
void new_vmu(void) {
    maple_device_t * dev;

    dev = maple_enum_dev(0, 1);

    if(dev == NULL) {
        if(dev_checked) {
            memset(vram_s + INFO_Y * SCREEN_W, 0, SCREEN_W * (SCREEN_H - 64) * 2);
            bfont_draw_str(vram_s + INFO_Y * SCREEN_W + 10, SCREEN_W, 0, "No VMU");
            dev_checked = 0;
        }
    }
    else if(dev_checked) {
    }
    else {
        memset(vram_s + INFO_Y * SCREEN_W, 0, SCREEN_W * (SCREEN_H - INFO_Y));
        draw_dir();
        dev_checked = 1;
    }
}

int wait_start(void) {
    maple_device_t *cont;
    cont_state_t *state;

    for(;;) {
        new_vmu();

        cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

        if(!cont) continue;

        state = (cont_state_t *)maple_dev_status(cont);

        if(!state)
            continue;

        if(state->buttons & CONT_START)
            return 0;
    }
}

static unsigned char vmu_icon[ICON_SIZE * NB_ICONS_MAX];

/* Here's the actual meat of it */
void write_entry(void) {
    vmu_pkg_t   pkg;
    uint8       data[4096], *pkg_out;
    int     pkg_size;
    int     i;
    file_t      f;

    strcpy(pkg.desc_short, "VMU Test");
    strcpy(pkg.desc_long, "This is a test VMU file");
    strcpy(pkg.app_id, "KOS");
    pkg.icon_cnt = NB_ICONS_MAX;
    pkg.icon_data = vmu_icon;
    pkg.icon_anim_speed = 8;
    pkg.eyecatch_type = VMUPKG_EC_NONE;

    for(i = 0; i < 4096; i++)
        data[i] = i & 255;

    vmu_pkg_load_icon(&pkg, "/rd/ebook.ico");
    vmu_pkg_build(&pkg, &pkg_out, &pkg_size);

    fs_unlink("/vmu/a1/TESTFILE");
    f = fs_open("/vmu/a1/TESTFILE", O_WRONLY);

    if(!f) {
        printf("error writing\n");
        return;
    }

    fs_write(f, data, sizeof(data));
    fs_vmu_set_header(f, &pkg);
    fs_close(f);
}

int main(int argc, char **argv) {
    bfont_draw_str(vram_s + 20 * SCREEN_W + 20, SCREEN_W, 0,
                   "Put a VMU you don't care too much about");
    bfont_draw_str(vram_s + 42 * SCREEN_W + 20, SCREEN_W, 0,
                   "in slot A1 and press START");
    bfont_draw_str(vram_s + INFO_Y * SCREEN_W + 10, SCREEN_W, 0, "No VMU");

    if(wait_start() < 0) return 0;

    write_entry();

    return 0;
}


