#include <kos.h>

static int x = 20, y = 20 + 24;

static void kb_test(void) {
    maple_device_t *cont, *kbd;
    cont_state_t *state;
    int k;

    printf("Now doing keyboard test\n");

    while(1) {
        cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

        if(!cont) continue;

        kbd = maple_enum_type(0, MAPLE_FUNC_KEYBOARD);

        if(!kbd) continue;

        /* Check for start on the controller */
        state = (cont_state_t *)maple_dev_status(cont);

        if(!state) {
            return;
        }

        if(state->buttons & CONT_START) {
            printf("Pressed start\n");
            return;
        }

        /* Get queued keys */
        while((k = kbd_queue_pop(kbd, 1)) != KBD_QUEUE_END) {
            if(k == 27) {
                printf("ESC pressed\n");
                return;
            }

            if(k > 0xff)
                printf("Special key %04x\n", k);

            if(k != 10) {
                bfont_draw(vram_s + y * 640 + x, 640, 0, k);
                x += 12;
                if(x >= 620) {
                    x = 20;
                    y += 24;
                }
            }
        }
    }
}

static void on_key_event(maple_device_t *dev, kbd_key_t key,
                         key_state_t state, kbd_mods_t mods,
                         kbd_leds_t leds, void *user_data) {
    kbd_state_t *kbd_state = (kbd_state_t *)dev->status;

    printf("[%c%u] %c: %s\n",
           'A' + dev->port, dev->unit,
           kbd_key_to_ascii(key, kbd_state->region, mods, leds),
           state.value == KEY_STATE_CHANGED_DOWN? "PRESSED" : "RELEASED");

    if(key == KBD_KEY_ENTER && state.value == KEY_STATE_CHANGED_DOWN) {
        x = 20;
        y += 24;
    }

}

int main(int argc, char **argv) {
    kbd_set_event_handler(on_key_event, NULL);

    for(int y = 0; y < 480; y++)
        for(int x = 0; x < 640; x++) {
            int c = (x ^ y) & 255;
            vram_s[y * 640 + x] = ((c >> 3) << 12)
                                  | ((c >> 2) << 5)
                                  | ((c >> 3) << 0);
        }

    kb_test();

    return 0;
}
