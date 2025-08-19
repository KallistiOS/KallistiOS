/* Here's a little "demo" with a new DC effect -- grayscale VMU! This
   uses a pretty common technique to get more than 2 colors out of a
   1 bit display -- page flipping. Basically you setup real refresh frames
   and a pixel should stay dark an amount of time proportional to its
   intended grayscale. Anyway, this implements that on the VMU. Thanks
   to Jordan for the idea =). If you don't believe your eyes, then hit
   the start button to stop the demo and you'll see what's left =).
*/

#include <kos.h>

/* This is really more complicated than it needs to be in this particular
   case, but it's nice and verbose. */
int check_start(void) {
    maple_device_t *cont;
    cont_state_t *state;

    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    if(!cont)
        return 0;

    /* Check for start on the controller */
    state = (cont_state_t *)maple_dev_status(cont);

    if(!state) {
        return 0;
    }

    if(state->buttons & CONT_START) {
        printf("Pressed start\n");
        return 1;
    }

    return 0;
}

/* Only this XPM will work with this code.. it's too cheap to actually
   do any parsing =) */
#include "graphic.xpm"

/* LCD Test: this will do a grayscale separation into several "frames" and
   flip through them quickly to give the illusion of grayscale on the LCD
   display. */
uint8_t lcd_disp[8][48 * 32 / 8];
void lcd_gs_pixel(int x, int y, int amt) {
    int i;

    for(i = 0; i < amt; i++)
        lcd_disp[i][(y * 48 + x) / 8] |= 0x80 >> (x & 7);
}
void lcd_gs_setup(void) {
    char **xpm = graphic_xpm + 12;  /* Skip header */
    int x, y;

    memset(lcd_disp, 0, sizeof(lcd_disp));

    for(y = 0; y < 32; y++) {
        for(x = 0; x < 48; x++) {
            /* Note that LCD images must be flipped! */
            int pixel = xpm[31 - y][47 - x];

            switch(pixel) {
                case '.':   /* White */
                    lcd_gs_pixel(x, y, 0);
                    break;
                case '#':   /* #e3e3e3 */
                    lcd_gs_pixel(x, y, 1);
                    break;
                case '+':   /* #c7c7c7 */
                    lcd_gs_pixel(x, y, 2);
                    break;
                case '@':   /* #aaaaaa */
                    lcd_gs_pixel(x, y, 3);
                    break;
                case '=':   /* #8e8e8e */
                case '-':   /* #727272 */
                    lcd_gs_pixel(x, y, 4);
                    break;
                case '$':   /* #555555 */
                    lcd_gs_pixel(x, y, 5);
                    break;
                case '*':   /* #393939 */
                    lcd_gs_pixel(x, y, 6);
                    break;
                case '&':   /* #1d1d1d */
                case '%':   /* #000000 */
                    lcd_gs_pixel(x, y, 7);
                    break;
            }
        }
    }
}

/* This performs the actual magic */
void lcd_test(void) {
    int frame = 0;

    lcd_gs_setup();

    while(!check_start()) {
        maple_device_t *addr = maple_enum_type(0, MAPLE_FUNC_LCD);

        if(addr) {
            int rv = vmu_draw_lcd(addr, lcd_disp + frame);

            if(rv < 0)
                printf("got error %d\n", rv);
            else {
                frame++;

                if(frame >= 8) frame = 0;
            }
        }

        // usleep(5 * 1000);
    }
}

int main(int argc, char **argv) {
    lcd_test();

    return 0;
}
