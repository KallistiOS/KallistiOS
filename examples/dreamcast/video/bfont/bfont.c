/* Very simple test for bfont (and its various encodings) */

#include <dc/biosfont.h>
#include <dc/video.h>
#include <dc/maple/controller.h>

#include <arch/arch.h>

#include <unistd.h>

int main(int argc, char **argv) {
    int x, y;

    for(y = 0; y < 480; y++)
        for(x = 0; x < 640; x++) {
            int c = (x ^ y) & 255;
            vram_s[y * 640 + x] = ((c >> 3) << 12)
                                  | ((c >> 2) << 5)
                                  | ((c >> 3) << 0);
        }

    /* Set our starting offset to one letter height away from the 
       top of the screen and two widths from the left */
    x = BFONT_WIDE_WIDTH;
    y = BFONT_HEIGHT;

    /* Test with ISO8859-1 encoding */
    bfont_set_encoding(BFONT_CODE_ISO8859_1);
    bfont_draw_str_vram_fmt(x, y, true, 
        "Test of basic ASCII\nParlez-vous fran�ais?");
    y += 2 * BFONT_HEIGHT;
    /* Do a second set drawn transparently */
    bfont_draw_str_vram_fmt(x, y, false,
        "Test of basic ASCII\nParlez-vous fran�ais?");

    y += 2 * BFONT_HEIGHT;

    /* Test with EUC encoding */
    bfont_set_encoding(BFONT_CODE_EUC);
    bfont_draw_str_vram_fmt(x, y, true, "����ˤ��� EUC!");
    y += BFONT_HEIGHT;
    bfont_draw_str_vram_fmt(x, y, false, "����ˤ��� EUC!");

    y += BFONT_HEIGHT;

    /* Test with Shift-JIS encoding */
    bfont_set_encoding(BFONT_CODE_SJIS);
    bfont_draw_str_vram_fmt(x, y, true, "�A�h���X�ϊ� SJIS");
    y += BFONT_HEIGHT;
    bfont_draw_str_vram_fmt(x, y, false, "�A�h���X�ϊ� SJIS");

    y += 2 * BFONT_HEIGHT;

    bfont_set_encoding(BFONT_CODE_ISO8859_1);
    bfont_draw_str_vram_fmt(x, y, true, 
        "We now support DC/VMU icons in printf statements!\n"
        "Use diXX for DC icons and viXX for VMU icons in your\n"
        "strings. Enhance your text with visual icons like \n" 
        "\\di03\\di04\\di05\\di06 for navigation, and make your UI more\n"
        "intuitive. You can display buttons like \\di0B\\di0C\\di0F\\di10\n"
        "\\di12\\di13 and icons like \\vi01\\vi05\\vi21\\vi18\\vi1C effortlessly.\n\n"
        "\tTo exit, press \\di14");

    /* If Start is pressed, exit the app */
    cont_btn_callback(0, CONT_START, (cont_btn_callback_t)arch_exit);

    /* Just trap here waiting for the button press */
    for(;;) { usleep(50); }

    return 0;
}
