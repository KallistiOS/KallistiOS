#include <kos.h>
#include <stdlib.h>

/* Display constants */
#define SCREEN_WIDTH        640
#define SCREEN_HEIGHT       480

void kb_test(void) {
    maple_device_t *cont, *kbd;
    cont_state_t *state;
    int k, x = 20, y = 20 + BFONT_HEIGHT;

    printf("Now doing keyboard test\n");

    while(true) {
        /* Query for the first detected controller */
        if((cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER))) {
            /* Fetch controller button state structure. */
            state = maple_dev_status(cont);

            /* Quit if start is pressed on the controller. */
            if(state->start) {
                printf("Pressed start!\n");
                return;
            }
        }

        kbd = maple_enum_type(0, MAPLE_FUNC_KEYBOARD);

        if(!kbd) continue;

        thd_sleep(10);

        /* Check for keyboard input */
        /* if (kbd_poll(mkb)) {
            printf("Error checking keyboard status\n");
            return;
        } */

        /* Keep popping keys while there are more enqueued. */
        while((k = kbd_queue_pop(kbd, true)) != KBD_QUEUE_END) {
            /* Quit if ESC key is pressed. */
            if(k == '\e') {
                printf("ESC pressed\n");
                return;
            }

            /* Log when special keys are pressed. */
            if(k > 0xff)
                printf("Special key %04x\n", k);

            /* Handle every key that isn't the RETURN key. */
            if(k != '\r') {
                /* Draw the key we just pressed. */
                bfont_draw(vram_s + y * SCREEN_WIDTH + x, SCREEN_WIDTH, 0, k);

                /* Advance the cursor horizontally. */
                x += BFONT_THIN_WIDTH;
            }
            else {
                x = 20;
                y += BFONT_HEIGHT;
            }
        }

        thd_sleep(10);
    }
}

int main(int argc, char **argv) {
    int x, y;

    for(y = 0; y < SCREEN_HEIGHT; y++)
        for(x = 0; x < SCREEN_WIDTH; x++) {
            int c = (x ^ y) & 255;
            vram_s[y * SCREEN_WIDTH + x] = ((c >> 3) << 12)
                                         | ((c >> 2) << 5)
                                         | ((c >> 3) << 0);
        }

    kb_test();

    return EXIT_SUCCESS;
}
