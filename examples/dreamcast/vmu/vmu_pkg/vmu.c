/* KallistiOS ##version##

   vmu.c
   (c)2002 Megan Potter
*/

/* This simple example shows how to use the vmu_pkg_* functions to write
   a file to a VMU with a DC-compatible header so it can be managed like
   any other VMU file from the BIOS menus. */

#include <kos.h>

void draw_dir(void) {
    file_t      d;
    int     y = 88;
    dirent_t    *de;

    d = fs_open("/vmu/a1", O_RDONLY | O_DIR);

    if(!d) {
        bfont_draw_str(vram_s + y * 640 + 10, 640, 0, "Can't read VMU");
    }
    else {
        while((de = fs_readdir(d))) {
            bfont_draw_str(vram_s + y * 640 + 10, 640, 0, de->name);
            y += 24;

            if(y >= (480 - 24))
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
            memset(vram_s + 88 * 640, 0, 640 * (480 - 64) * 2);
            bfont_draw_str(vram_s + 88 * 640 + 10, 640, 0, "No VMU");
            dev_checked = 0;
        }
    }
    else if(dev_checked) {
    }
    else {
        memset(vram_s + 88 * 640, 0, 640 * (480 - 88));
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
    pkg.icon_cnt = 0;
    pkg.icon_anim_speed = 0;
    pkg.eyecatch_type = VMUPKG_EC_NONE;
    pkg.data_len = 4096;
    pkg.data = data;

    for(i = 0; i < 4096; i++)
        data[i] = i & 255;

    vmu_pkg_build(&pkg, &pkg_out, &pkg_size);

    fs_unlink("/vmu/a1/TESTFILE");
    f = fs_open("/vmu/a1/TESTFILE", O_WRONLY);

    if(!f) {
        printf("error writing\n");
        return;
    }

    fs_write(f, pkg_out, pkg_size);
    fs_close(f);
}

int main(int argc, char **argv) {
    bfont_draw_str(vram_s + 20 * 640 + 20, 640, 0,
                   "Put a VMU you don't care too much about");
    bfont_draw_str(vram_s + 42 * 640 + 20, 640, 0,
                   "in slot A1 and press START");
    bfont_draw_str(vram_s + 88 * 640 + 10, 640, 0, "No VMU");

    if(wait_start() < 0) return 0;

    write_entry();

    return 0;
}


