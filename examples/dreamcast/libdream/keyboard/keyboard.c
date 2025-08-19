#include <kos.h>
#include <stdlib.h>

/* Display constants */
#define SCREEN_WIDTH        640
#define SCREEN_HEIGHT       480
#define MARGIN_HORIZONTAL   20
#define MARGIN_VERTICAL     20
#define PATTERN_OFFSET      64

/* Structure holding cursor state */
typedef struct {
    int x;
    int y;
    mutex_t m;
} cursor_t;

/* Function for performing textual input style polling logic by
   popping keys off of the pending keyboard queue to process.
*/
static void kb_test(cursor_t *cursor) {
    maple_device_t *cont, *kbd;
    cont_state_t *state;
    int k;

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
            else if(k != '\r') {
                mutex_lock(&cursor->m);
                /* Draw the key we just pressed. */
                bfont_draw(vram_s + cursor->y * SCREEN_WIDTH + cursor->x,
                           SCREEN_WIDTH, 0, k);

                /* Advance the cursor horizontally. */
                cursor->x += BFONT_THIN_WIDTH;

                /* Implement wrapping if we're at the end of the line. */
                if(cursor->x >= (SCREEN_WIDTH - MARGIN_HORIZONTAL)) {
                    cursor->x = MARGIN_HORIZONTAL;
                    cursor->y += BFONT_HEIGHT;
                }

                mutex_unlock(&cursor->m);
            }
        }
    }
}

/* Asynchronous callback function which is invoked by the keyboard
   driver any time a key's state changes. This is what you want to
   use when using the keyboard as a traditional controller-like input-
   device.
*/
static void on_key_event(maple_device_t *dev, kbd_key_t key,
                         key_state_t state, kbd_mods_t mods,
                         kbd_leds_t leds, void *user_data) {
    /* Retrieve keyboard state from maple device. */
    kbd_state_t *kbd_state = kbd_get_state(dev);
    /* Fetch cursor from generic userdata pointer. */
    cursor_t *cursor = user_data;

    /* Print keyboard address + key ID and state change type. */
    printf("[%c%u] %c: %s\n",
           'A' + dev->port, dev->unit,
           kbd_key_to_ascii(key, 1, mods, leds),
           state.value == KEY_STATE_CHANGED_DOWN? "PRESSED" : "RELEASED");

    /* Check whether the RETURN key was pressed (but not held): */
    if(key == KBD_KEY_ENTER && state.value == KEY_STATE_CHANGED_DOWN) {
        if(mutex_trylock(&cursor->m)) return;
        /* Advance cursor to the next line. */
        cursor->x = MARGIN_HORIZONTAL;
        cursor->y += BFONT_HEIGHT;
        mutex_unlock(&cursor->m);
    }
}

int main(int argc, char **argv) {
    int x, y;

    /* Initialize our cursor in the top-left. */
    cursor_t cursor = {
        .x = MARGIN_HORIZONTAL,
        .y = MARGIN_VERTICAL + BFONT_HEIGHT,
        .m = ERRORCHECK_MUTEX_INITIALIZER
    };

    for(y = 0; y < SCREEN_HEIGHT; y++)
        for(x = 0; x < SCREEN_WIDTH; x++) {
            int c = ((x % PATTERN_OFFSET) ^
                     (y % PATTERN_OFFSET)) & 0xff;
            vram_s[y * SCREEN_WIDTH + x] = ((c >> 3) << 12)
                                         | ((c >> 2) << 5)
                                         | ((c >> 3) << 0);
        }

    /* Install a custom keyboard event handler which will listen for
       key state changes, passing it our cursor. This drives the keyboard
       as a controller-like input device.
    */
    kbd_set_event_handler(on_key_event, &cursor);

    /* Run our main logic which drives the keyboard as a text-input device. */
    kb_test(&cursor);

    return EXIT_SUCCESS;
}
