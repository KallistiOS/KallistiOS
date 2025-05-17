/* KallistiOS ##version##

   vmu_game.c
   (c)2020 BBHoodsta
*/

/* This simple example shows how to use the vmufs_write function to write
   a VMU game file to a VMU with a DC-compatible header so it can be played on the vmu. */

#include <kos.h>

void draw_findings(void) {
    file_t      d;

    d = fs_open("/vmu/a1", O_RDONLY | O_DIR);

    if(!d) {
        bfont_draw_str_vram_fmt(10, 88, false, "Can't read VMU");
    }
    else {
        bfont_draw_str_vram_fmt(10, 88, false, "VMU found. Press Start.");
    }
}

int dev_checked = 0;
void new_vmu(void) {
    maple_device_t * dev;

    dev = maple_enum_dev(0, 1);

    if(dev == NULL) {
        if(dev_checked) {
            memset(vram_s + 88 * 640, 0, 640 * (480 - 64) * 2);
            bfont_draw_str_vram_fmt(10, 88, false, "No VMU");
            dev_checked = 0;
        }
    }
    else if(dev_checked) {
    }
    else {
        memset(vram_s + 88 * 640, 0, 640 * (480 - 88));
        draw_findings();
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
void write_game_entry(void) {
    file_t f;
    int data_size;
    uint8_t *data;
    maple_device_t *dev;

    f = fs_open("/rd/TETRIS.VMS", O_RDONLY);

    if(!f) {
        printf("Error reading Tetris game from romdisk\n");
        return;
    }

    data_size = fs_total(f);
    data = (uint8_t*) malloc(data_size + 1);
    fs_read(f, data, data_size);
    fs_close(f);

    dev = maple_enum_type(0, MAPLE_FUNC_MEMCARD);
    
    if(dev)
        vmufs_write(dev, "Tetris", data, data_size, VMUFS_VMUGAME);
}

int main(int argc, char **argv) {
    bfont_draw_str_vram_fmt(20, 20, false,
        "Put a VMU you don't care too much about\nin slot A1 and press START\n\nNo VMU");

    if(wait_start() < 0) return 0;

    write_game_entry();

    return 0;
}


