/*  KallistiOS ##version##

    examples/dreamcast/pvr/yuv_converter/YUV422/yuv422.c
    Copyright (C) 2023 Andy Barajas

    This example shows how to use TA's YUV converter for YUV422p format.
    A sample YUV422p image in the romdisk was made via ffmpeg:
    
       ffmpeg -i 422.png -pix_fmt yuv422p 422.yuv
       
    Note: This example is for YUV422 images in Y, U, V plane order. Hence
    the p in YUV422p.
    
    PVR Register Config:
    
    1. Set destination for YUV conversion results:
       PVR_SET(PVR_YUV_ADDR, (((unsigned int)pvr_txr_address) & 0xffffff));
       
    2. Define the type and size of conversion:
       PVR_SET(PVR_YUV_CFG, (0x01 << 24) | 
               (((PVR_TEXTURE_HEIGHT / 16) - 1) << 8) | 
               ((PVR_TEXTURE_WIDTH / 16) - 1));
               
    The PVR_YUV_CFG register specifies conversion type and resulting image 
    dimensions. The bit pattern (0x01 << 24) indicates YUV422; for YUV420,
    it would be (0x00 << 24).
    
    PVR_GET(PVR_YUV_CFG) can read this register's value. The docs recommend it,
    so it's good practice to include it.

    This example utilizes the function convert_YUV422_to_YUV422_texture() to 
    feed the YUV converter the necessary data in macro blocks. Although DMA 
    can be used for this transfer, store queues are used by default as 
    they offer better performance in this context.
*/

#include <stdio.h>
#include <stdlib.h>
#include <arch/arch.h>
#include <arch/cache.h>

#include <dc/pvr.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

#define PVR_TEXTURE_WIDTH 512
#define PVR_TEXTURE_HEIGHT 512

/* The image dimensions can be different than the dimensions of the pvr
   texture BUT the dimensions have to be a multiple of 16 */
#define FRAME_TEXTURE_WIDTH 512
#define FRAME_TEXTURE_HEIGHT 512

/* u_block + v_block + y_block = 128 + 128 + 256 = 512 */
#define BYTE_SIZE_FOR_16x16_BLOCK 512

static pvr_poly_hdr_t hdr;
static pvr_vertex_t vert[4];
static pvr_ptr_t pvr_txr;

static uint8_t *y_plane;
static uint8_t *u_plane;
static uint8_t *v_plane;

static int load_image(void) {
    FILE *file = fopen("/rd/422.yuv", "rb");
    size_t read_size = 0;

    if(file == NULL) {
        printf("Could not open the file.\n");
        goto error;
    }

    y_plane = aligned_alloc(32, FRAME_TEXTURE_WIDTH * FRAME_TEXTURE_HEIGHT);
    u_plane = aligned_alloc(32, FRAME_TEXTURE_WIDTH * FRAME_TEXTURE_HEIGHT / 2);
    v_plane = aligned_alloc(32, FRAME_TEXTURE_WIDTH * FRAME_TEXTURE_HEIGHT / 2);

    if(!y_plane || !u_plane || !v_plane) {
        printf("Could not allocate memory for y,u, or v plane\n");
        goto error;
    }

    read_size = fread(y_plane, 1, FRAME_TEXTURE_WIDTH * FRAME_TEXTURE_HEIGHT, 
                      file);
    if(read_size != FRAME_TEXTURE_WIDTH * FRAME_TEXTURE_HEIGHT) {
        printf("Could not read y_plane completely\n");
        goto error;
    }

    read_size = fread(u_plane, 1, FRAME_TEXTURE_WIDTH * FRAME_TEXTURE_HEIGHT / 2, 
                      file);
    if(read_size != FRAME_TEXTURE_WIDTH * FRAME_TEXTURE_HEIGHT / 2) {
        printf("Could not read u_plane completely\n");
        goto error;
    }

    read_size = fread(v_plane, 1, FRAME_TEXTURE_WIDTH * FRAME_TEXTURE_HEIGHT / 2, 
                      file);
    if(read_size != FRAME_TEXTURE_WIDTH * FRAME_TEXTURE_HEIGHT / 2) {
        printf("Could not read v_plane completely\n");
        goto error;
    }

    fclose(file);
    return 0;

error:
    if(file)
        fclose(file);

    if(y_plane)
        free(y_plane);

    if(u_plane)
        free(u_plane);

    if(v_plane)
        free(v_plane);

    return 1;
}

static int setup_pvr(void) {
    pvr_poly_cxt_t cxt;

    if(!(pvr_txr = pvr_mem_malloc(PVR_TEXTURE_WIDTH * PVR_TEXTURE_HEIGHT * 2))) {
        printf("Failed to allocate PVR memory!\n");
        return -1;
    }

    /* Set SQ to YUV converter. */
    PVR_SET(PVR_YUV_ADDR, (((unsigned int)pvr_txr) & 0xffffff));
    /* Divide PVR texture width and texture height by 16 and subtract 1. */
    PVR_SET(PVR_YUV_CFG, (0x01 << 24) | /* Set bit to specify 422 data format */
                         (((PVR_TEXTURE_HEIGHT / 16) - 1) << 8) | 
                         ((PVR_TEXTURE_WIDTH / 16) - 1));
    /* Need to read once. */
    PVR_GET(PVR_YUV_CFG);

    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, 
                    PVR_TXRFMT_YUV422 | PVR_TXRFMT_NONTWIDDLED, 
                    PVR_TEXTURE_WIDTH, PVR_TEXTURE_HEIGHT, 
                    pvr_txr, 
                    PVR_FILTER_BILINEAR);
    pvr_poly_compile(&hdr, &cxt);

    hdr.mode3 |= PVR_TXRFMT_STRIDE;

    vert[0].z     = vert[1].z     = vert[2].z     = vert[3].z     = 1.0f; 
    vert[0].argb  = vert[1].argb  = vert[2].argb  = vert[3].argb  = 
        PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);    
    vert[0].oargb = vert[1].oargb = vert[2].oargb = vert[3].oargb = 0;  
    vert[0].flags = vert[1].flags = vert[2].flags = PVR_CMD_VERTEX;         
    vert[3].flags = PVR_CMD_VERTEX_EOL;

    float width_ratio = (float)FRAME_TEXTURE_WIDTH / PVR_TEXTURE_WIDTH;
    float height_ratio = (float)FRAME_TEXTURE_HEIGHT / PVR_TEXTURE_HEIGHT;

    vert[0].x = 0.0f;
    vert[0].y = 0.0f;
    vert[0].u = 0.0f;
    vert[0].v = 0.0f;

    vert[1].x = 640.0f;
    vert[1].y = 0.0f;
    vert[1].u = width_ratio;
    vert[1].v = 0.0f;

    vert[2].x = 0.0f;
    vert[2].y = 480.0f;
    vert[2].u = 0.0f;
    vert[2].v = height_ratio;

    vert[3].x = 640.0f;
    vert[3].y = 480.0f;
    vert[3].u = width_ratio;
    vert[3].v = height_ratio;

    return 0;
}

static void convert_YUV422_to_YUV422_texture(void) {
    int i, j, index, x_blk, y_blk;

    unsigned char u_block[128] __attribute__((aligned(32)));
    unsigned char v_block[128] __attribute__((aligned(32)));
    unsigned char y_block[256] __attribute__((aligned(32)));

    for(y_blk = 0; y_blk < FRAME_TEXTURE_HEIGHT; y_blk += 16) {
        for(x_blk = 0; x_blk < FRAME_TEXTURE_WIDTH; x_blk += 16) {

            /* U data for 2 (16x8 pixels) */
            for(i = 0; i < 16; ++i) {
                index = (y_blk + i) * (FRAME_TEXTURE_WIDTH / 2) + (x_blk / 2);
                *((uint64_t*)&u_block[i * 8]) = *((uint64_t*)&u_plane[index]);
            }

            /* V data for 2 (16x8 pixels) */
            for(i = 0; i < 16; ++i) {
                index = (y_blk + i) * (FRAME_TEXTURE_WIDTH / 2) + (x_blk / 2);
                *((uint64_t*)&v_block[i * 8]) = *((uint64_t*)&v_plane[index]);
            }

            /* Y data for 4 (8x8 pixels) */
            for(i = 0; i < 4; ++i) {
                for(j = 0; j < 8; ++j) {
                    index = (y_blk + j + (i / 2 * 8)) * FRAME_TEXTURE_WIDTH + 
                             x_blk + (i % 2 * 8);
                    *((uint64_t*)&y_block[i * 64 + j * 8]) = 
                        *((uint64_t*)&y_plane[index]);
                }
            }

            // dcache_flush_range((uint32_t)u_block, 64);
            // pvr_dma_yuv_conv( (void*)u_block, 64, 1, NULL, 0);
            pvr_sq_load((void *)0, (void *)u_block, 64, PVR_DMA_YUV);

            // dcache_flush_range((uint32_t)v_block, 64);
            // pvr_dma_yuv_conv( (void*)v_block, 64, 1, NULL, 0);
            pvr_sq_load((void *)0, (void *)v_block, 64, PVR_DMA_YUV);

            // dcache_flush_range((uint32_t)y_block, 128);
            // pvr_dma_yuv_conv( (void*)y_block, 128, 1, NULL, 0);
            pvr_sq_load((void *)0, (void *)y_block, 128, PVR_DMA_YUV);

            // dcache_flush_range((uint32_t)(u_block+64), 64);
            // pvr_dma_yuv_conv( (void*)(u_block+64), 64, 1, NULL, 0);
            pvr_sq_load((void *)0, (void *)(u_block+64), 64, PVR_DMA_YUV);

            // dcache_flush_range((uint32_t)(v_block+64), 64);
            // pvr_dma_yuv_conv( (void*)(v_block+64), 64, 1, NULL, 0);
            pvr_sq_load((void *)0, (void *)(v_block+64), 64, PVR_DMA_YUV);

            // dcache_flush_range((uint32_t)(y_block+128), 128);
            // pvr_dma_yuv_conv( (void*)(y_block+128), 128, 1, NULL, 0);
            pvr_sq_load((void *)0, (void *)(y_block+128), 128, PVR_DMA_YUV);
        }

        /* Send dummies if frame texture width doesn't match pvr texture width */
        pvr_sq_set32((void *)PVR_TA_YUV_CONV, 0, 
                     BYTE_SIZE_FOR_16x16_BLOCK * 
                     ((PVR_TEXTURE_WIDTH >> 4) - (FRAME_TEXTURE_WIDTH >> 4)), PVR_DMA_YUV);
    }
}

static void show_image(void) {

    pvr_wait_ready();
    pvr_scene_begin();

    pvr_list_begin(PVR_LIST_OP_POLY);
    pvr_prim(&hdr, sizeof(pvr_poly_hdr_t));
    pvr_prim(&vert[0], sizeof(pvr_vertex_t));
    pvr_prim(&vert[1], sizeof(pvr_vertex_t));
    pvr_prim(&vert[2], sizeof(pvr_vertex_t));
    pvr_prim(&vert[3], sizeof(pvr_vertex_t));
    pvr_list_finish();

    pvr_scene_finish();
}

static void __attribute__((__noreturn__)) wait_exit(void) {
    maple_device_t *dev;
    cont_state_t *state;

    printf("Press any button to exit.\n");

    for(;;) {
        dev = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

        if(dev) {
            state = (cont_state_t *)maple_dev_status(dev);

            if(state)   {
                if(state->buttons)
                    arch_exit();
            }
        }
    }
}

int main(int argc, char *argv[]) {
    pvr_init_defaults();

    if(load_image() != 0)
        return -1;

    if(setup_pvr() != 0)
        return -1;

    convert_YUV422_to_YUV422_texture();

    show_image();

    /* Free all allocated memory */
    free(y_plane);
    free(u_plane);
    free(v_plane);
    pvr_mem_free(pvr_txr);

    wait_exit();

    return 0;
}
