/* This sample program shows off 640x480, but it also shows off "quiet
   initialization" which is where you tell KOS to shut up upon
   startup. This can be useful if you don't want to do serial_disable
   because you need the port, but you don't want garbage being printed
   when it's successful either. */

#include <kos.h>

int main(int argc, char **argv) {
    int x, y;

    /* Press all buttons to exit */
    cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
                      (cont_btn_callback_t)arch_exit);

    /* Bother us with output only if something died */
    dbglog_set_level(DBG_DEAD);

    /* Set the video mode */
    vid_set_mode(DM_640x480, PM_RGB565);

    for(y = 0; y < 480; y++)
        for(x = 0; x < 640; x++) {
            int c = (x ^ y) & 255;
            vram_s[y * 640 + x] = ((c >> 3) << 12)
                                  | ((c >> 2) << 5)
                                  | ((c >> 3) << 0);
        }

    printf("\n\nPress all buttons simultaneously to exit.\n");
    fflush(stdout);
    while(1);

    return 0;
}
