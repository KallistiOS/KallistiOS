/* KallistiOS ##version##
   strided_texture.c
   Copyright (C) 2024 Andress Barajas
*/

/*
    This example demonstrates rendering a black-and-white chessboard pattern
    using a 640x480 texture with 16bpp color depth. In this example, the texture
    width and stride are both set to 640. However, because 640 is not a power
    of two, we need to use the `PVR_TXRFMT_X32_STRIDE` flag. This flag informs the
    PVR that the texture's width (or "stride") is a multiple of 32 rather than
    a power of two.
    Steps to configure and render a texture with a 32-pixel multiple width:

    1. Configure the Polygon Header for Textures:

       - Use `pvr_poly_cxt_txr()` to set up the polygon context for the texture.
       - Specify `PVR_TXRFMT_NONTWIDDLED | PVR_TXRFMT_X32_STRIDE` in the format
         flags.
       - Provide dimensions in powers of two, which should be larger than the
         actual texture. For example, if your texture is 640x480, set width and
         height to `1024` and `512`, respectively.
       Example:
           ```c
           pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY,
                            PVR_TXRFMT_RGB565 | PVR_TXRFMT_NONTWIDDLED |
                            PVR_TXRFMT_X32_STRIDE, 1024, 512,
                            texture_pointer, PVR_FILTER_NONE);
           ```
    2. Set the Global Texture Stride Register:
       - Use `pvr_txr_set_stride(texture_width);` to define a custom texture
         stride width in increments of 32 pixels.
       - The `texture_width` parameter should be the full width of the texture
         in pixels.
       - This setting instructs the hardware on how to interpret each row's
         width in VRAM for non-power-of-two textures. For a 640-pixel wide
         texture, you would call:

           ```c
           pvr_txr_set_stride(640);
           ```
    Important Notes:
       - Texture widths that are multiples of 32 (but not powers of two) require
         the `PVR_TXRFMT_X32_STRIDE` flag.
       - Palette-based textures are incompatible with the `PVR_TXRFMT_X32_STRIDE`
         flag, as are mipmaps as both require the texture format to be twiddled.
       - `pvr_txr_set_stride()` sets a global PVR register. All textures using
         the `PVR_TXRFMT_X32_STRIDE` flag in the same frame must share the same
         stride. Changing `pvr_txr_set_stride()` affects all such textures
         rendered afterward.
*/

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include <dc/pvr.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

/* The width of the texture in pixels (must be a multiple of 32) */
#define TEXTURE_WIDTH    640

/* The height of the texture in pixels */
#define TEXTURE_HEIGHT   480

/*
   Macro to calculate the next power of two for a given integer `x`.

   Example:
     NEXT_POWER_OF_TWO(640) -> 1024
*/
#define NEXT_POWER_OF_TWO(x)    (1 << (32 - __builtin_clz((x) - 1)))

/*
   The power-of-two width to be passed to texture setup functions.
   Based on TEXTURE_WIDTH, this should be equal to or greater than
   TEXTURE_WIDTH, and will match the hardware requirement for
   power-of-two texture dimensions.
*/
#define TEXTURE_PADDED_WIDTH    NEXT_POWER_OF_TWO(TEXTURE_WIDTH)

/*
   The power-of-two height to be passed to texture setup functions.
   Based on TEXTURE_HEIGHT, this should be equal to or greater than
   TEXTURE_HEIGHT, following the power-of-two requirement.
*/
#define TEXTURE_PADDED_HEIGHT   NEXT_POWER_OF_TWO(TEXTURE_HEIGHT)

/* RGB565 colors for chessboard pattern */
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF

static pvr_poly_hdr_t hdr;
static pvr_vertex_t verts[4];

static pvr_ptr_t board_texture;

static void draw_frame(void) {
    pvr_wait_ready();
    pvr_scene_begin();

    pvr_list_begin(PVR_LIST_OP_POLY);

    pvr_prim(&hdr, sizeof(hdr));

    pvr_prim(&verts[0], sizeof(pvr_vertex_t));
    pvr_prim(&verts[1], sizeof(pvr_vertex_t));
    pvr_prim(&verts[2], sizeof(pvr_vertex_t));
    pvr_prim(&verts[3], sizeof(pvr_vertex_t));

    pvr_list_finish();
    pvr_scene_finish();
}

static void load_texture(void) {
    pvr_poly_cxt_t cxt;
    uint16_t *grid_texture;

    /* Allocate memory for the texture with 32-byte alignment */
    grid_texture = (uint16_t *)aligned_alloc(32, TEXTURE_WIDTH * TEXTURE_HEIGHT * 2);

    /* Generate a chessboard pattern */
    for(int y = 0; y < TEXTURE_HEIGHT; y++) {
        for(int x = 0; x < TEXTURE_WIDTH; x++) {
            /* Determine if we are in an even or odd square. */
            int square_x = x / 32;
            int square_y = y / 32;
            int is_white_square = (square_x + square_y) % 2;

            grid_texture[y * TEXTURE_WIDTH + x] = is_white_square
                                    ? COLOR_WHITE : COLOR_BLACK;
        }
    }

    /* Allocate space in VRAM for the texture */
    board_texture = pvr_mem_malloc(TEXTURE_WIDTH * TEXTURE_HEIGHT * 2);

    /* Load the chessboard pattern into VRAM */
    pvr_txr_load(grid_texture, board_texture,
                 TEXTURE_WIDTH * TEXTURE_HEIGHT * 2);

    /* Set texture context format for nontwiddled, strided texture */
    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY,
                     PVR_TXRFMT_RGB565 | PVR_TXRFMT_NONTWIDDLED
                     | PVR_TXRFMT_X32_STRIDE, TEXTURE_PADDED_WIDTH,
                     TEXTURE_PADDED_HEIGHT, board_texture, PVR_FILTER_NONE);
    pvr_poly_compile(&hdr, &cxt);

    /* Set the global non-power-of-two stride register */
    pvr_txr_set_stride(TEXTURE_WIDTH);

    free(grid_texture);
}

/*
   When setting up the vertices, the texture width (stride) is divided by
   the padded texture width (and similarly for the height) to ensure that
   the non-power-of-two dimensions are mapped correctly to the power-of-two
   padded dimensions used in VRAM. This allows textures with non-standard
   widths (e.g., multiples of 32) to render accurately on the Dreamcast's
   PVR hardware.
*/
static void setup_vertices(void) {
    int color = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);

    verts[0].x = 0.0f;
    verts[0].y = 0.0f;
    verts[0].z = 1.0f;
    verts[0].u = 0.0f;
    verts[0].v = 0.0f;
    verts[0].argb = color;
    verts[0].oargb = 0;
    verts[0].flags = PVR_CMD_VERTEX;

    verts[1].x = 640.0f;
    verts[1].y = 0.0f;
    verts[1].z = 1.0f;
    verts[1].u = (float)TEXTURE_WIDTH / (float)TEXTURE_PADDED_WIDTH;
    verts[1].v = 0.0f;
    verts[1].argb = color;
    verts[1].oargb = 0;
    verts[1].flags = PVR_CMD_VERTEX;

    verts[2].x = 0.0f;
    verts[2].y = 480.0f;
    verts[2].z = 1.0f;
    verts[2].u = 0.0f;
    verts[2].v = (float)TEXTURE_HEIGHT / (float)TEXTURE_PADDED_HEIGHT;
    verts[2].argb = color;
    verts[2].oargb = 0;
    verts[2].flags = PVR_CMD_VERTEX;

    verts[3].x = 640.0f;
    verts[3].y = 480.0f;
    verts[3].z = 1.0f;
    verts[3].u = (float)TEXTURE_WIDTH / (float)TEXTURE_PADDED_WIDTH;
    verts[3].v = (float)TEXTURE_HEIGHT / (float)TEXTURE_PADDED_HEIGHT;
    verts[3].argb = color;
    verts[3].oargb = 0;
    verts[3].flags = PVR_CMD_VERTEX_EOL;
}

int main(int argc, char **argv) {

    pvr_init_defaults();

    /* If the user hits start, bail */
    cont_btn_callback(0, CONT_START, (cont_btn_callback_t)exit);

    load_texture();

    setup_vertices();

    draw_frame();

    /* Wait for exit */
    for(;;) { }

    return 0;
}
