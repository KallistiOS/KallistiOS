/* KallistiOS ##version##

   keytest.c
   Copyright (C) 2018 Donald Haase

The Purpose of this program is to provide some testing of the basic keyboard functionality.

Currently it merely takes in a preset number of printable test characters. This allows for the testing
of basic US keyboard functionality, appropriate shift handling, and the newer key repeating feature.

Room has been explicitly left open for further tests. It might be useful to include:
-Correct handling of multiple keyboards.
-Handling of non-US keyboards.
-Handling of mod keys and lights.
-Output of keyboard info for comparison to hardware.


*/
#define WIDTH 640
#define HEIGHT 480
#define STARTLINE 20
#define CHARSPERLINE 40
#define CHARSPERTEST 120

#include <assert.h>
#include <kos.h>

KOS_INIT_FLAGS(INIT_DEFAULT);
extern uint16_t *vram_s;

cont_state_t* first_kbd_state;
maple_device_t* first_kbd_dev = NULL;

/* Track how many times we try to find a keyboard before just quitting. */
uint8_t no_kbd_loop = 0;
/* This is set up to have multiple tests in the future. */
uint8_t test_phase = 0;


void basic_typing (void)
{
    int charcount = 0;
    int rv;
    int lines = 0;
    uint32_t offset = ((STARTLINE+(lines*BFONT_HEIGHT)) * WIDTH);
    bfont_draw_str(vram_s + offset, WIDTH, 1, "Test of basic typing. Enter 120 characters: ");
    offset = ((STARTLINE+((++lines)*BFONT_HEIGHT)) * WIDTH);

    while (charcount < CHARSPERTEST) {
        rv = kbd_queue_pop(first_kbd_dev, 1);
        if(rv<0) continue;

        bfont_draw(vram_s + offset, WIDTH, 1, (char)rv);
        offset += BFONT_THIN_WIDTH;
        charcount++;
        if(!(charcount%CHARSPERLINE)) offset = ((STARTLINE+((++lines)*BFONT_HEIGHT)) * WIDTH);
    }
    return;
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
