/* KallistiOS ##version##

   draw.c
   Copyright (C) 2002 Megan Potter

   Adapted from Kosh, Copyright (C) 2000 Jordan DeLong

*/

#include <assert.h>
#include <dc/pvr.h>
#include <dc/biosfont.h>
#include <conio/conio.h>

/* the font texture */
static pvr_ptr_t font_texture = NULL;

/* initialize draw stuff: get our texture of the font, etc */
void conio_draw_init(void) {
    uint16_t *vram;
    int x, y;

    font_texture = pvr_mem_malloc(256*256*2);
    vram = (uint16_t *)font_texture;

    for (y = 0; y < 8; y++) {
        for (x = 0; x < 16; x++) {
            bfont_draw(vram, 256, 0, y*16 + x);
            vram += 16;
        }
        vram += 23*256;
    }
}

void conio_draw_shutdown(void) {
    assert( font_texture != NULL );
    pvr_mem_free(font_texture);
}

/* Draw one font character (6x12) */
static void draw_char(float x1, float y1, float z1, float a, float r,
                      float g, float b, int c) {
    pvr_vertex_t vert;
    int ix, iy;
    float u1, v1, u2, v2;

    if (c == ' ')
        return;

    // assert( c > ' ' && c < 127 );
    if (!( c > ' ' && c < 127 ))
        return;

    ix = (c % 16) * 16;
    iy = (c / 16) * 24;
    u1 = ix * 1.0f / 256.0f;
    v1 = iy * 1.0f / 256.0f;
    u2 = (ix+12) * 1.0f / 256.0f;
    v2 = (iy+24) * 1.0f / 256.0f;

    vert.flags = PVR_CMD_VERTEX;
    vert.x = x1;
    vert.y = y1 + CONIO_FONT_HEIGHT;
    vert.z = z1;
    vert.u = u1;
    vert.v = v2;
    vert.argb = PVR_PACK_COLOR(a, r, g, b);
    vert.oargb = 0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = x1;
    vert.y = y1;
    vert.u = u1;
    vert.v = v1;
    pvr_prim(&vert, sizeof(vert));

    vert.x = x1 + CONIO_FONT_WIDTH;
    vert.y = y1 + CONIO_FONT_HEIGHT;
    vert.u = u2;
    vert.v = v2;
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = x1 + CONIO_FONT_WIDTH;
    vert.y = y1;
    vert.u = u2;
    vert.v = v1;
    pvr_prim(&vert, sizeof(vert));
}

/* draw len chars at string */
static void draw_string(float x, float y, float z, float a, float r, float g,
        float b, char *str, int len) {
    int i;
    pvr_poly_cxt_t  cxt;
    pvr_poly_hdr_t  poly;

    pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED,
        256, 256, font_texture, PVR_FILTER_NONE);
    pvr_poly_compile(&poly, &cxt);
    pvr_prim(&poly, sizeof(poly));

    for (i = 0; i < len; i++) {
        draw_char(x, y, z, a, r, g, b, str[i]);
        x += CONIO_FONT_WIDTH;
    }
}

/* draw a box (used by cursor and border, etc) (at 1.0f z coord) */
static void draw_box(float x, float y, float w, float h, float a, float r, float g, float b) {
    pvr_poly_cxt_t  cxt;
    pvr_poly_hdr_t  poly;
    pvr_vertex_t    vert;

    pvr_poly_cxt_col(&cxt, PVR_LIST_TR_POLY);
    pvr_poly_compile(&poly, &cxt);
    pvr_prim(&poly, sizeof(poly));

    vert.flags = PVR_CMD_VERTEX;
    vert.x = x;
    vert.y = y + h;
    vert.z = 1.0f;
    vert.u = vert.v = 0.0f;
    vert.argb = PVR_PACK_COLOR(a, r, g, b);
    vert.oargb = 0;
    pvr_prim(&vert, sizeof(vert));

    vert.y -= h;
    pvr_prim(&vert, sizeof(vert));

    vert.y += h;
    vert.x += w;
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.y -= h;
    pvr_prim(&vert, sizeof(vert));
}

/* draw the border around the screen etc */
static void draw_border(float r, float g, float b) {
    draw_box(0, 0, CONIO_SCREEN_FIRSTCOL - 1, CONIO_SCREEN_HEIGHT, 1.0f, r, g, b);
    draw_box(CONIO_SCREEN_LASTCOL, 0, CONIO_SCREEN_WIDTH - CONIO_SCREEN_LASTCOL, CONIO_SCREEN_HEIGHT, 1.0f, r, g, b);
    draw_box(0, 0, CONIO_SCREEN_WIDTH, CONIO_FONT_HEIGHT, 1.0f, r, g, b);
    draw_box(0, CONIO_SCREEN_HEIGHT - CONIO_FONT_HEIGHT, CONIO_SCREEN_WIDTH, CONIO_FONT_HEIGHT, 1.0f, r, g, b);
}

static void draw_middle(float r, float g, float b) {
    draw_box(CONIO_SCREEN_FIRSTCOL, CONIO_FONT_HEIGHT, CONIO_FONT_WIDTH * CONIO_NUM_COLS, CONIO_FONT_HEIGHT * CONIO_NUM_ROWS, 1.0f, r, g, b);
}

/* draw the virtual screen */
static void draw_virtscr(float a, float r, float g, float b) {
    int i;

    for (i = 0; i < CONIO_NUM_ROWS; i++) {
        draw_string(CONIO_SCREEN_FIRSTCOL, (i + 1) * CONIO_FONT_HEIGHT, 1.0f, a, r, g, b,
            conio_virtscr[i], CONIO_NUM_COLS);
    }
}

/* draw the cursor */
static float c_alpha = 1.0f, c_dalpha = -0.1f;
static void draw_cursor(float r, float g, float b) {
    /* modify our alpha to give a little fading effect */
    c_alpha += c_dalpha;
    if (c_alpha >= 1.0f || c_alpha <= 0.0f)
        c_dalpha = -c_dalpha;

    if (c_alpha > 0.5f) {
        draw_box(conio_cursor.col * CONIO_FONT_WIDTH + CONIO_SCREEN_FIRSTCOL, (conio_cursor.row + 1) * CONIO_FONT_HEIGHT,
            CONIO_FONT_WIDTH, CONIO_FONT_HEIGHT,
            1.0f, r, g, b);
    }
}

/* our exported drawing function: does a full redraw of everything */
void conio_draw_frame(void) {
    pvr_wait_ready();
    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_TR_POLY);
    switch (conio_theme) {
        case CONIO_THEME_C64:
            draw_border(0.4f, 0.6f, 0.98f);
            draw_middle(0.0f, 0.0f, 1.0f);
            draw_virtscr(1.0f, 1.0f, 1.0f, 1.0f);
            draw_cursor(0.4f, 0.6f, 0.98f);
            break;
        case CONIO_THEME_MATRIX:
            draw_virtscr(1.0f, 0.0f, 1.0f, 0.0f);
            draw_cursor(0.0f, 0.7f, 0.0f);
            break;
        case CONIO_THEME_PLAIN:
        default:
            draw_virtscr(1.0f, 1.0f, 1.0f, 1.0f);
            draw_cursor(1.0f, 1.0f, 1.0f);
            break;
    }
    pvr_list_finish();
    pvr_scene_finish();
}
