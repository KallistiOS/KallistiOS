/* KallistiOS ##version##

   keyrawtest.c
   Copyright (C) 2018 Donald Haase
   Copyright (C) 2025 Troy Davis

   This example demonstrates raw keyboard event handling on the Dreamcast,
   expanding on the original keytest example.

   Improvements include:
   - Uses kbd_queue_pop(dev, 0) to capture all key events, not just ASCII-typed ones.
   - Displays the current LED state (Caps Lock, Num Lock, Scroll Lock) for each event.
   - Logs raw scancodes and attempts to name special keys for debugging purposes.
   - Displays printable ASCII characters on screen, as in the original example.

   This test is useful for:
   - Verifying key mappings and modifier behavior.
   - Confirming proper LED state toggling and reporting.
   - Debugging keyboard driver improvements or hardware compatibility.

   Future enhancements could include:
   - Multiple keyboard support.
   - Non-US layout testing.
   - Enhanced visual feedback or UI.
*/

#define WIDTH 640
#define HEIGHT 480
#define STARTLINE 20
#define CHARSPERLINE 40
#define CHARSPERTEST 120

#include <assert.h>
#include <kos.h>

extern uint16_t *vram_s;

static cont_state_t *first_kbd_state;
static maple_device_t *first_kbd_dev = NULL;

/* Track how many times we try to find a keyboard before just quitting. */
static uint8_t no_kbd_loop = 0;
/* This is set up to have multiple tests in the future. */
static uint8_t test_phase = 0;

// Add this helper function above basic_typing()
static const char *kbd_key_name(kbd_key_t key) {
    switch (key) {
        case KBD_KEY_NONE: return "NONE";
        case KBD_KEY_ERROR: return "ERROR";
        case KBD_KEY_ERR2: return "ERR2";
        case KBD_KEY_ERR3: return "ERR3";
        case KBD_KEY_ENTER: return "ENTER";
        case KBD_KEY_ESCAPE: return "ESC";
        case KBD_KEY_BACKSPACE: return "BACKSPACE";
        case KBD_KEY_TAB: return "TAB";
        case KBD_KEY_SPACE: return "SPACE";
        case KBD_KEY_CAPSLOCK: return "CAPSLOCK";
        case KBD_KEY_INSERT: return "INSERT";
        case KBD_KEY_HOME: return "HOME";
        case KBD_KEY_PGUP: return "PGUP";
        case KBD_KEY_DEL: return "DEL";
        case KBD_KEY_END: return "END";
        case KBD_KEY_PGDOWN: return "PGDOWN";
        case KBD_KEY_RIGHT: return "RIGHT";
        case KBD_KEY_LEFT: return "LEFT";
        case KBD_KEY_DOWN: return "DOWN";
        case KBD_KEY_UP: return "UP";
        case KBD_KEY_PRINT: return "PRINT";
        case KBD_KEY_SCRLOCK: return "SCRLOCK";
        case KBD_KEY_PAUSE: return "PAUSE";
        case KBD_KEY_F1: return "F1";
        case KBD_KEY_F2: return "F2";
        case KBD_KEY_F3: return "F3";
        case KBD_KEY_F4: return "F4";
        case KBD_KEY_F5: return "F5";
        case KBD_KEY_F6: return "F6";
        case KBD_KEY_F7: return "F7";
        case KBD_KEY_F8: return "F8";
        case KBD_KEY_F9: return "F9";
        case KBD_KEY_F10: return "F10";
        case KBD_KEY_F11: return "F11";
        case KBD_KEY_F12: return "F12";
        case KBD_KEY_PAD_NUMLOCK: return "PAD_NUMLOCK";
        case KBD_KEY_PAD_DIVIDE: return "PAD_DIVIDE";
        case KBD_KEY_PAD_MULTIPLY: return "PAD_MULTIPLY";
        case KBD_KEY_PAD_MINUS: return "PAD_MINUS";
        case KBD_KEY_PAD_PLUS: return "PAD_PLUS";
        case KBD_KEY_PAD_ENTER: return "PAD_ENTER";
        case KBD_KEY_PAD_0: return "PAD_0";
        case KBD_KEY_PAD_1: return "PAD_1";
        case KBD_KEY_PAD_2: return "PAD_2";
        case KBD_KEY_PAD_3: return "PAD_3";
        case KBD_KEY_PAD_4: return "PAD_4";
        case KBD_KEY_PAD_5: return "PAD_5";
        case KBD_KEY_PAD_6: return "PAD_6";
        case KBD_KEY_PAD_7: return "PAD_7";
        case KBD_KEY_PAD_8: return "PAD_8";
        case KBD_KEY_PAD_9: return "PAD_9";
        case KBD_KEY_PAD_PERIOD: return "PAD_PERIOD";
        case KBD_KEY_S3: return "S3";
        default: return NULL;
    }
}

static void basic_typing(void)
{
    int charcount = 0;
    int lines = 0;
    uint32_t offset = ((STARTLINE + (lines * BFONT_HEIGHT)) * WIDTH);
    bfont_draw_str(vram_s + offset, WIDTH, 1, "Test of raw typing. Enter 120 keys: ");
    offset = ((STARTLINE + ((++lines) * BFONT_HEIGHT)) * WIDTH);

    while (charcount < CHARSPERTEST) {
        int raw = kbd_queue_pop(first_kbd_dev, 0);
        if(raw == KBD_QUEUE_END) continue;

        // Decode raw: 0x00FF = key, 0xFF00 = modifiers, 0xFF0000 = LEDs
        kbd_key_t key = (kbd_key_t)(raw & 0xFF);
        kbd_mods_t mods = { .raw = (raw >> 8) & 0xFF };
        kbd_leds_t leds = { .raw = (raw >> 16) & 0xFF };

        kbd_state_t *kbd = maple_dev_status(first_kbd_dev);
        if(!kbd) continue;

        char ascii = kbd_key_to_ascii(key, kbd->region, mods, leds);

        printf("LEDs: caps=%d num=%d scroll=%d\n", leds.caps_lock, leds.num_lock, leds.scroll_lock);

        // Show printable ASCII characters on screen
        if(ascii >= 32 && ascii <= 126) {
            bfont_draw(vram_s + offset, WIDTH, 1, ascii);
            offset += BFONT_THIN_WIDTH;
            charcount++;
            if(!(charcount % CHARSPERLINE)) {
                offset = ((STARTLINE + ((++lines) * BFONT_HEIGHT)) * WIDTH);
            }
        }

        // Log everything
        char debug[128];
        const char *keyname = kbd_key_name(key);

        if(ascii >= 32 && ascii <= 126) {
            snprintf(debug, sizeof(debug),
                "RAW 0x%02X | ascii: %c | shift:%d caps:%d ctrl:%d alt:%d s1:%d s2:%d",
                key, ascii,
                mods.lshift || mods.rshift,
                leds.caps_lock,
                mods.lctrl || mods.rctrl,
                mods.lalt || mods.ralt,
                mods.s1,
                mods.s2
            );
        } else if(keyname) {
            snprintf(debug, sizeof(debug),
                "RAW 0x%02X | key: %s | shift:%d caps:%d ctrl:%d alt:%d s1:%d s2:%d",
                key, keyname,
                mods.lshift || mods.rshift,
                leds.caps_lock,
                mods.lctrl || mods.rctrl,
                mods.lalt || mods.ralt,
                mods.s1,
                mods.s2
            );
        } else {
            snprintf(debug, sizeof(debug),
                "RAW 0x%02X | key: 0x%02X | shift:%d caps:%d ctrl:%d alt:%d s1:%d s2:%d",
                key, key,
                mods.lshift || mods.rshift,
                leds.caps_lock,
                mods.lctrl || mods.rctrl,
                mods.lalt || mods.ralt,
                mods.s1,
                mods.s2
            );
        }

        printf("%s\n", debug);
    }
}

int main(int argc, char **argv)
{
    for(;;) {
        /* If the dev is null, refresh it. */
        while(first_kbd_dev == NULL) {
            first_kbd_dev = maple_enum_type(0, MAPLE_FUNC_KEYBOARD);
            /* If it's *still* null, wait a bit and check again. */
            if(first_kbd_dev == NULL)   {
                timer_spin_sleep(500);
                no_kbd_loop++;
            }
            if( no_kbd_loop >= 25 ) return -1;
        }
        /* Reset the timeout counter */
        no_kbd_loop = 0;

        first_kbd_state = (cont_state_t *) maple_dev_status(first_kbd_dev);
        if(first_kbd_state == NULL) assert_msg(0, "Invalid Keyboard state returned");

        if(test_phase == 0)
            basic_typing();
        else
            break;

        test_phase++;
    }
    return 0;
}