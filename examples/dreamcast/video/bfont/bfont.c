/* Very simple test for bfont (and its various encodings) */

#include <dc/biosfont.h>
#include <dc/video.h>
#include <dc/maple/controller.h>

#include <arch/arch.h>

#include <unistd.h>

int main(int argc, char **argv) {
    int x, y, o;

    for(y = 0; y < 480; y++)
        for(x = 0; x < 640; x++) {
            int c = (x ^ y) & 255;
            vram_s[y * 640 + x] = ((c >> 3) << 12)
                                  | ((c >> 2) << 5)
                                  | ((c >> 3) << 0);
        }

    /* Set our starting offset to one letter height away from the 
       top of the screen and two widths from the left */
    o = (640 * BFONT_HEIGHT) + (BFONT_THIN_WIDTH * 2);

    /* Test with ASCII encoding */
    bfont_set_encoding(BFONT_CODE_ASCII);
    bfont_draw_str(vram_s + o, 640, 1, "Test of basic ASCII");
    /* After each string, we'll increment the offset down by one row */
    o += 640 * BFONT_HEIGHT;
    bfont_draw_str(vram_s + o, 640, 1, "The brackets below should be empty:");
    o += 640 * BFONT_HEIGHT;
    /* These characters are all part of ISO 8859-1 and not in ASCII */
    bfont_draw_str(vram_s + o, 640, 1, "[¡Àæ¾ç]");
    o += 640 * BFONT_HEIGHT;


    /* Test with ISO8859-1 encoding */
    bfont_set_encoding(BFONT_CODE_ISO8859_1);
    bfont_draw_str(vram_s + o, 640, 1, "ISO8859-1 math symbols: «±×÷¼½¾»");
    o += 640 * BFONT_HEIGHT;
    bfont_draw_str(vram_s + o, 640, 1, "Parlez-vous français?");
    o += 640 * BFONT_HEIGHT;

    /* Do a second set drawn transparently */
    bfont_draw_str(vram_s + o, 640, 0, "ISO8859-1 math symbols: «±×÷¼½¾»");
    o += 640 * BFONT_HEIGHT;
    bfont_draw_str(vram_s + o, 640, 0, "Você fala português?");
    o += 640 * BFONT_HEIGHT;

    /* Test with EUC encoding */
    bfont_set_encoding(BFONT_CODE_EUC);
    bfont_draw_str(vram_s + o, 640, 1, "¤³¤ó¤Ë¤Á¤Ï EUC!");
    o += 640 * BFONT_HEIGHT;
    bfont_draw_str(vram_s + o, 640, 0, "¤³¤ó¤Ë¤Á¤Ï EUC!");
    o += 640 * BFONT_HEIGHT;

    /* Test with Shift-JIS encoding */
    bfont_set_encoding(BFONT_CODE_SJIS);
    bfont_draw_str(vram_s + o, 640, 1, "ƒAƒhƒŒƒX•ÏŠ· SJIS");
    o += 640 * BFONT_HEIGHT;
    bfont_draw_str(vram_s + o, 640, 0, "ƒAƒhƒŒƒX•ÏŠ· SJIS");
    o += 640 * BFONT_HEIGHT;

    /* Drawing the special symbols is a bit convoluted. First we'll draw some
       standard text as above. */
    bfont_set_encoding(BFONT_CODE_ISO8859_1);
    bfont_draw_str(vram_s + o, 640, 1, "To exit, press ");

    /* Then we set the mode to raw to draw the special character. */
    bfont_set_encoding(BFONT_CODE_RAW);
    /* Adjust the writing to start after "To exit, press " and draw the one char */
    bfont_draw_wide(vram_s + o + (BFONT_THIN_WIDTH * 15), 640, 1, BFONT_STARTBUTTON);

    /* If Start is pressed, exit the app */
    cont_btn_callback(0, CONT_START, (cont_btn_callback_t)arch_exit);

    /* Just trap here waiting for the button press */
    for(;;) { usleep(50); }

    return 0;
}
