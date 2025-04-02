/* KallistiOS ##version##

   pvr_internal.h
   Copyright (C) 2002, 2003, 2004 Megan Potter

 */

#ifndef __PVR_INTERNAL_H
#define __PVR_INTERNAL_H

/* Various implementation details are contained in here; this should only ever
   be included by modules in this directory.

   Everything from here down is considered internal to the implementation
   and may change without notice. So please don't rely on it in your
   code. If something is needed from this, an external interface should
   be added to dc/pvr.h. */

#include <stdbool.h>
#include <kos/mutex.h>

/**** State stuff ***************************************************/

/* The internal workings of the PVR2 are quite complex, and thank goodness
   we have the TA to help us with this setup process for each frame, or
   it'd be a LOT more work!

   Basically you have three different sets of buffers while registering
   scene data:

   1) Vertex buffer: this is a PVR RAM buffer that holds processed vertex
      data as it is fed to the TA
   2) Object pointer buffer: this is essentially an array of lists which
      holds data about which objects appear may appear in which tiles; for
      some odd reason, it grows down (probably so you don't have to pre-size
      vertex and OPB buffers, kinda like heap and stack)
   3) Tile matrix: this has a fixed-size entry for each tile on that will
      be rendered to; each active list must have a pointer into the OPB,
      or an "end of list" marker to mark it as have no OPB space

   As the TA collects data, the buffers may start to overflow if you have
   a lot of polygons, and that's what the grow space is about. It is
   initially using a fairly small amount of PVR RAM to hold the data
   structures, but as it overflows the bins for each tile, it must
   allocate a new block.

   3D processing proceeds in a pipeline fashion. There are four functional
   units we have to consider in this process: the main CPU, the tile
   accelerator, the ISP/TSP, and the visual output.

   If vertex DMA is enabled, then the TA may optionally be fed by the CPU,
   which will free it up from stalls that may happen with certain polygons
   when feeding the TA, as well as enabling other benefits.

   So in an ideal situation with no DMA enabled, it looks like this:

   VBlanks  SH4-to-TA   ISP/TSP         View
   0        ->T0        -           -
   1        ->T1        T0->F0          -
   2        ->T0        T1->F1          F0
   3        ->T1        T0->F0          F1
   ...

   When vertex DMA is enabled, we go into a naive 3-stage setup. This can
   be improved later, but it's a start for now.

   In this mode, we augment the timing diagram above:

   VBlanks  SH4-to-RAM  DMA-to-TA   ISP/TSP         View
   0        ->R0        -       -           -
   1        ->R1        R0->T0      -           -
   2        ->R0        R1->T1      T0->F0          -
   3        ->R1        R0->T0      T1->F1          F0
   4        ->R0        R1->T1      T0->F0          F1
   ...

   In the current naive implementation, everything is timed off of vblank
   interrupts. So the program can write vertices to the RAM buffers as long
   as it wants. On the first vblank where the current RAM buffers are filled
   up, DMA proceeds from the filled buffer to the TA. On the first vblank
   where all the TA transfers have completed, ISP/TSP rendering is started.
   On the first vblank where a frame has been completed, the view is switched
   to the frame. Thus everything sort of cascades in natural order when it's
   ready. This also solves the issue in previous versions where one would
   write a single frame and it'd never show up unless you push through
   several more frames. For example, a single frame written would look
   like this:

   VBlanks  SH4-to-RAM  DMA-to-TA   ISP/TSP         View
   0        ->R0        -       -           -
   1        -       R0->T0      -           -
   2        -       -       T0->F0          -
   3        -       -       -           F0

   Another example, if the CPU spent more than 16msec generating data in the
   SH4-to-RAM phase, it might look like this at 30fps:

   VBlanks  SH4-to-RAM  DMA-to-TA   ISP/TSP         View
   0        ->R0        -       -           -
   1        -       R0->T0      -           -
   2        ->R1        -       T0->F0          -
   3        -       R1->T1      -           F0
   4        ->R0        -       T1->F1          F0
   5        -       R0->T0      -           F1
   6        ->R1        -       T0->F0          F1
   ...

   Note that in the case where the potentially bigger frames cause the DMA-to-TA
   or ISP/TSP phases to take longer than one frame, they are allowed to expand
   into the next slot gracefully.

 */

/* Note that these must match the list types in pvr.h; these are here
   mainly because they're easier to type =) */
#define PVR_OPB_OP      0   /* Array indices for these structures */
#define PVR_OPB_OM      1
#define PVR_OPB_TP      2
#define PVR_OPB_TM      3
#define PVR_OPB_PT      4
#define PVR_OPB_COUNT   5

// TA buffers structure: we have two sets of these
typedef struct {
    uint32  vertex, vertex_size;            /* Vertex buffer */
    uint32  opb, opb_size;                  /* Object pointer buffers, size */
    uint32  opb_addresses[PVR_OPB_COUNT];        /* Object pointer buffers (of each type) */
    uint32  tile_matrix, tile_matrix_size;  /* Tile matrix, size */
    uint32  opb_overflow_count;             /* Extra OPB space after opb_size for TA overflow */
} pvr_ta_buffers_t;

// DMA buffers structure: we have two sets of these
typedef struct {
    uint8   * base[PVR_OPB_COUNT];  // DMA buffers, if assigned
    uint32  ptr[PVR_OPB_COUNT];     // DMA buffer write pointer, if used
    uint32  size[PVR_OPB_COUNT];    // DMA buffer sizes, or zero if none
    int ready;                      // >0 if these buffers are ready to be DMAed
} pvr_dma_buffers_t;

// Frame buffers structure: we have two sets of these
typedef struct {
    uint32  frame, frame_size;      // Output frame buffer, size
} pvr_frame_buffers_t;

/* PVR status structure; not only will this hold status information,
   but it will also server as the wait object for the frame-complete
   genwaits. */
typedef struct {
    // If this is zero, then this state isn't valid
    int     valid;

    // General configuration
    uint32  lists_enabled;              // opb_completed's value when we're ready to render
    uint32  list_reg_mask;              // Active lists register mask
    int     dma_mode;                   // 1 if we are using DMA to transfer vertices
    int     opb_size[PVR_OPB_COUNT];    // opb size flags

    // Pipeline state
    int     ram_target;                 // RAM buffer we're writing into
                                        // (^1 == RAM buffer we're DMAing from)
    int     ta_target;                  // TA buffer we're writing (or DMAing) into
                                        // (^1 == TA buffer we're rendering from)
    int     view_target;                // Frame buffer we're viewing
                                        // (^1 == frame buffer we're rendering to)

    int     list_reg_open;              // Which list is open for registration, if any? (non-DMA only)
    uint32  lists_closed;               // (1 << idx) for each list which the SH4 has lost interest in
    uint32  lists_transferred;          // (1 << idx) for each list which has completely transferred to the TA
    uint32  lists_dmaed;                // (1 << idx) for each list which has been DMA'd (DMA mode only)

    mutex_t dma_lock;                   // Locked if a DMA is in progress (vertex or texture)
    int     ta_ready;                   // >0 if the TA is ready for the new scene
    int     ta_busy;                    // >0 if a scene is ongoing and the TA hasn't signaled completion
    int     render_busy;                // >0 if a render is in progress
    int     render_completed;           // >1 if a render has recently finished

    // Memory pointers / buffers
    pvr_dma_buffers_t   dma_buffers[2];     // DMA buffers (if any)
    pvr_ta_buffers_t    ta_buffers[2];      // TA buffers
    pvr_frame_buffers_t frame_buffers[2];   // Frame buffers
    uint32              texture_base;       // Start of texture RAM

    // Screen size / clipping constants
    int     w, h;                       // Screen width, height
    int     tw, th;                     // Screen tile width, height
    uint32  tsize_const;                // Screen tile size constant
    float   zclip;                      // Z clip plane
    uint32  pclip_left, pclip_right;    // X pixel clip constants
    uint32  pclip_top, pclip_bottom;    // Y pixel clip constants
    uint32  pclip_x, pclip_y;           // Composited clip constants
    uint32  bg_color;                   // Background color in ARGB format

    /* Running statistics on the PVR system. All vars are in terms
       of nanoseconds. */
    uint64_t frame_last_time;            // When did the last frame completion occur?
    uint64_t buf_start_time;             // When did the last DMA buffer fill begin?
    uint64_t reg_start_time;             // When did the last registration begin?
    uint64_t rnd_start_time;             // When did the last render begin?
    uint64_t frame_last_len;             // VBlank-to-VBlank length for the last frame (1.0/FrameRate)
    uint64_t buf_last_len;               // Cumulative buffer fill time for the last frame
    uint64_t reg_last_len;               // Registration time for the last frame
    uint64_t rnd_last_len;               // Render time for the last frame
    size_t   vbl_count;                  // VBlank counter for animations and such
    size_t   frame_count;                // Total number of viewed frames
    size_t   vtx_buf_used;               // Vertex buffer used size for the last frame
    size_t   vtx_buf_used_max;           // Maximum used vertex buffer size

    // Handle for the vblank interrupt
    int     vbl_handle;

    // Non-zero if FSAA was enabled at init time.
    int     fsaa;

    // Non-zero if using double-buffering for the vertex buffer.
    int     vbuf_doublebuf;

    // True if the next frame is rendered to a texture
    bool    next_to_texture;

    // True if the frame processed by the TA is rendered to a texture
    bool    curr_to_texture;

    // True if the frame processed by the CORE is rendered to a texture
    bool    was_to_texture;

    // Render pitch for to-texture mode for the current frame
    int     to_txr_rp;

    // Render pitch for to-texture mode for the next frame
    int     next_to_txr_rp;

    // Output address for to-texture mode for the current frame
    uint32  to_txr_addr;

    // Output address for to-texture mode for the next frame
    uint32  next_to_txr_addr;

    // Whether direct rendering is active or not
    uint32  dr_used;
} pvr_state_t;

/* There will be exactly one of these in KOS (in pvr_globals.c) */
extern volatile pvr_state_t pvr_state;

/* Background plane structure */
typedef struct pvr_bkg_poly {
    uint32      flags1, flags2;
    uint32      dummy;
    float       x1, y1, z1;
    uint32      argb1;
    float       x2, y2, z2;
    uint32      argb2;
    float       x3, y3, z3;
    uint32      argb3;
} pvr_bkg_poly_t;

// Debug macro, for debugging IRQ wackiness
#define DBG(x) do { \
        int o = irq_disable(); \
        printf x; \
        irq_restore(o); \
    } while(0)

/**** pvr_buffers.c ***************************************************/

/* Initialize buffers for TA/ISP/TSP usage */
void pvr_allocate_buffers(const pvr_init_params_t *params);

/* Fill the tile matrices (after it's initialized) */
void pvr_init_tile_matrices(bool presort);


/**** pvr_misc.c ******************************************************/

/* What event is happening (for pvr_sync_stats)? */
#define PVR_SYNC_VBLANK     1   /* VBlank IRQ */
#define PVR_SYNC_BUFSTART   2   /* DMA buffer fill started */
#define PVR_SYNC_BUFDONE    3   /* DMA buffer fill complete */
#define PVR_SYNC_REGSTART   4   /* Registration started */
#define PVR_SYNC_REGDONE    5   /* Registration complete */
#define PVR_SYNC_RNDSTART   6   /* Render started */
#define PVR_SYNC_RNDDONE    7   /* Render complete IRQ */
#define PVR_SYNC_PAGEFLIP   8   /* View page was flipped */

/* Update statistical counters */
void pvr_sync_stats(int event);

/* Synchronize the viewed page with what's in pvr_state */
void pvr_sync_view(void);

/* Synchronize the registration buffer with what's in pvr_state */
void pvr_sync_reg_buffer(void);

/* Begin a render operation that has been queued completely */
void pvr_begin_queued_render(void);

/* Generate synthetic polygon headers for the given list type (to submit
   blank lists that the user forgot) */
void pvr_blank_polyhdr(int type);

/* Same as above, but generates into a buffer instead of submitting. */
void pvr_blank_polyhdr_buf(int type, pvr_poly_hdr_t * buf);


/**** pvr_irq.c *******************************************************/

/* Interrupt handlers for PVR events */
void pvr_int_handler(uint32 code, void *data);
void pvr_vblank_handler(uint32 code, void *data);

void pvr_start_dma(void);

#endif
