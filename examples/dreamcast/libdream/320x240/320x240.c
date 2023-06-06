#include <kos.h>

#define W 320
#define H 240

int main(int argc, char **argv) {
    int x, y;

    /* Press all buttons to exit */
    cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
                      (cont_btn_callback_t)arch_exit);

    /* Set the video mode */
    vid_set_mode(DM_320x240, PM_RGB565);

    for(y = 0; y < 240; y++)
        for(x = 0; x < 320; x++) {
            int c = (x ^ y) & 255;
            vram_s[y * 320 + x] = ((c >> 3) << 12)
                                  | ((c >> 2) << 5)
                                  | ((c >> 3) << 0);
        }

    for(y = 0; y < H; y += 24) {
        char tmp[16];
        sprintf(tmp, "%d", y);
        bfont_draw_str(vram_s + y * W + 10, W, 0, tmp);
    }

    for(x = 0; x < W; x += 100) {
        char tmp[16];
        sprintf(tmp, "%d", x / 10);
        bfont_draw_str(vram_s + 10 * W + x, W, 0, tmp);
    }

    printf("\n\nPress all buttons simultaneously to exit.\n");
    fflush(stdout);
    while(1);

    return 0;
}

