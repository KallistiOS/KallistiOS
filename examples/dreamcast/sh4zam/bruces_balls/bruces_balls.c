/*! \file
    \brief Example of pushing polygons with SH4ZAM.

    Welcome to Bruce's Balls!

    The following example illustrates the use of SH4ZAM to accelerate
    rendering and vertex processing. It also serves as a performance
    benchmark at the high-end, capable of pushing 4.5 million polygons
    per second with the proper build flags.

    The program simply renders a series of high-polygon balls and reports
    back the rendering performance in terms of polygons per second. It
    does so by using KOS's "Direct Rendering" PVR API, which submits geometry
    to the PowerVR GPU via filling and flushing the two store queues to the
    TA in alternation. Ths is the fastest rendering path. 

    \note
    This example is written in C23.

    \author 2025 BruceLeet
    \author 2025, 2026 Falco Girgis

*/

#include <kos.h>
#include <sh4zam/shz_sh4zam.h>

#include <stdlib.h>

//! Number of balls to render (play with me!)
constexpr unsigned BALL_COUNT    = 93;
//! Number of Z stacks which comprise the ball's geometry
constexpr unsigned SPHERE_STACKS = 20;
//! Number of X/Y slices which comprise each stack of the ball's geometry
constexpr unsigned SPHERE_SLICES = 20;

//! Total number of triangles which get rendered per ball.
constexpr unsigned TRIANGLES_PER_BALL = SPHERE_STACKS * SPHERE_SLICES * 2;
//! Total number of triangles rendered per scene, including all balls.
constexpr unsigned TRIANGLES_TOTAL    = TRIANGLES_PER_BALL * BALL_COUNT;

//! Structure representing a single ball.
typedef struct {
    alignas(32)             // Align to cache-line sizes
        shz_vec3_t pos;     //!< 3D Position of a ball.
        shz_vec2_t rot;     //!< X/Y orientation of a ball.
        shz_vec2_t vel;     //!< X/Y velocity of a ball.
        uint32_t   color;   //!< ARGB8888 color of a ball.
} Ball;

// The secret to high-performance is keeping your data cache-friendly.
static_assert(sizeof(Ball)  == 32); // Ensure we are exactly the size of a cache-line.
static_assert(alignof(Ball) == 32); // Ensure we are exactly aligned to start on cache-line boundaries.

/*! Precomputes and caches the polygon header used for drawing balls.

    For maximum performance, do not waste time dynamically compiling
    polygon contexts into headers each frame. Do it once and cache 
    the result!
*/
static void cache_polygon_header(pvr_poly_hdr_t* poly_header) {
    pvr_poly_cxt_t cxt;

    /* Preallocate the cache line which maps to the polygon header.

        NOTE: You need to be SUPER careful with this. You can only do this without
        screwing up your data when it fits perfectly into cache lines, or else you
        can corrupt any other data within the cache line.

        It is fine here, because KOS aligns these headers to cache-line sizes.
    */
    shz_dcache_alloc_line(poly_header);
    // Initialize the polygon context for an untextured opaque polygon.
    pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
    //! Compile the polygon context into a polygon header for use later.
    pvr_poly_compile(poly_header, &cxt);
}

//! Submits the polygon header for the ball vertex strip to the PowerVR GPU.
static void submit_polygon_header(const pvr_poly_hdr_t* poly_header) {
    /* Ensure the header is resident within the cache before shz_sq_memcpy32() uses it.
       
       NOTE: Do not do this blindly. You can only have one prefetch in-flight at a time,
       and it takes approximately 10-12 cycles for the prefetch to complete before the
       data is ready. Accessing the data befoere the prefetch completes OR issuing a second
       overlapping prefetch results in stalling the SH4 pipeline until the prefetch completes.
    */
    SHZ_PREFETCH(poly_header);
    // Optimized routine for copying a single 32-byte chunk of data via the SH4 Store Queues.
    shz_sq_memcpy32_1(pvr_dr_target(), poly_header);
}

//! Sets up and caches our projection view matrix for use later.
static void setup_projection_view(shz_mat4x4_t* mat) {
    constexpr float screen_width  = 640.0f;
    constexpr float screen_height = 480.0f;
    constexpr float near_z        = 0.0f;
    constexpr float fov           = SHZ_DEG_TO_RAD(60.0f);   
    constexpr float aspect        = screen_width / screen_height;
    
    // Preallocate the cache line containing the first 32-bytes of the matrix.
    shz_dcache_alloc_line(mat);
    /* Initialize XMTRX to a permutation matrix which swizzles the order of any
       vector which gets transformed by it. We do this so that, given an 4D vector
       we are transforming to submit to the GPU, shz_xmtrx_transform_vec4() produces
       the W component first, which is what we need immediately for perspective
       division of the other components.
    */
    shz_xmtrx_init_permutation_wxyz();
    /* Apply a screen-space transform, scaling the vertex positions to the viewport.
       
       This is because the final positions used for rendering by the PVR need to be
       in screen-space rather than normalized device coordinates!
       
       NOTE: KOS's mat_perspective() does this automatically for you, but we do it
       explicitly in SH4ZAM for fine-grained control.
    
    */
    shz_xmtrx_apply_screen(screen_width, screen_height);
    // Allocate the cache line above the second half of the matrix.
    shz_dcache_alloc_line(((void*)mat) + 32);
    /* Apply a perspective projection matrix.
    
       NOTE: far_z is automatically expected to be FLOAT_MAX, so it
       is not explicitly configured here.
    */
    shz_xmtrx_apply_perspective(fov, aspect, near_z);
    // Store the resulting matrix for later.
    shz_xmtrx_store_4x4(mat);
}

/*! Ensures the final transform for a given ball occupies XMTRX.

    This is the matrix which each position vector of a ball gets
    transformed against.
*/
static void apply_model_matrix(shz_vec3_t pos, 
                               shz_vec2_t rot, 
                               const shz_mat4x4_t* proj_view) {
    // Load the cached projection view matrix.
    shz_xmtrx_load_4x4(proj_view);
    // Apply a translation matrix to position the ball in 3D space.
    shz_xmtrx_translate(pos.x, pos.y, -pos.z);
    // Apply a rotation matrix to rotate the ball about the X axis.
    shz_xmtrx_apply_rotation_x(rot.x);
    // Apply a rotation matrix to rotate the ball about the Y axis.
    shz_xmtrx_apply_rotation_y(rot.y);
}

//! Renders a single one of Bruce's balls.
static void render_sphere(float radius, uint32_t base_color) {
    // Precompute values used to compute the balls' angles.
    constexpr float stackStep = SHZ_F_PI / SPHERE_STACKS;
    constexpr float sliceStep = SHZ_F_PI * 2.0f / SPHERE_SLICES;
    
    /* "Warm up" the two store queues by pre-initializing the color values of both.
        This is an advanced technique whereby you can avoid having to write data
        unnecessarily to the store queues which remains constant for every vertex.

        Since there are only two store queues which we alternate between using, and
        every vertex in the current ball has the same color, we might as well only
        set its values a single time.
    */
    ((pvr_vertex_t*)pvr_dr_target())->argb = base_color;
    ((pvr_vertex_t*)pvr_dr_target())->argb = base_color;

    // Iterate over each Z stack that we're drawing for a given ball.
    for(unsigned stack = 0; stack < SPHERE_STACKS; stack++) {
        // Compute the angle of the current and next stacks/
        const float stackAngle = SHZ_F_PI_2 - stack * stackStep;
        const float nextStackAngle = SHZ_F_PI_2 - (stack + 1) * stackStep;
        
        // Retrieve both the sin and cosine for the given angles as pairs.
        shz_sincos_t sc1 = shz_sincosf(stackAngle);
        shz_sincos_t sc2 = shz_sincosf(nextStackAngle);
        
        // Compute Z positions and radii for two vertices.
        const float z1 = sc1.sin;
        const float r1 = sc1.cos;
        const float z2 = sc2.sin;
        const float r2 = sc2.cos;
        
        /* Warm up the SQs by setting the vertex command, which is held constant until
           the very last vertex is submitted, which needs to send an end-of-list command.
        */
        ((pvr_vertex_t*)pvr_dr_target())->flags = PVR_CMD_VERTEX;
        ((pvr_vertex_t*)pvr_dr_target())->flags = PVR_CMD_VERTEX;

        // Render each X/Y sphere slice for the given ball.
        for(unsigned slice = 0; slice < SPHERE_SLICES; slice++) {
            // Calculate the angle of rotation for the given slice.
            const float sliceAngle = slice * sliceStep;
            // Compute the sin/cos pairs for the angle of the current slice.
            shz_sincos_t sc_slice = shz_sincosf(sliceAngle);

            // Compute the local position of the first vertex.
            shz_vec3_t local_pos1 = shz_vec3_scale(shz_vec3_init(sc_slice.cos * r2, sc_slice.sin * r2, z2), radius);
            /* Transform the local position by the 4x4 transform matrix currently held within XMTRX.

                NOTE: We must extend the 3D position into 4D space in order to get a transformed W
                coordinate for perspective division!
            */
            shz_vec4_t trans_pos1 = shz_xmtrx_transform_vec4(shz_vec3_vec4(local_pos1, 1.0f));
            // Since we applied a WXYZ swizzle to the projection matrix to get the W component first, we have to deswizzle.
            trans_pos1 = shz_vec4_swizzle(trans_pos1, 1, 2, 3, 0);

            // Map a PVR vertex pointer into the SH4 store queues, so we can fill it in.
            pvr_vertex_t* vert1 = pvr_dr_target();

            /* Prevent GCC from reordering our memory accesses in a way which causes us to access the transformed position
               before it's ready, which would cause a pipeline stall while waiting for the transform to complete. */
            SHZ_MEMORY_BARRIER_SOFT();

            /* Use a fast inversion approximation trick to QUICKLY find the inverse of our W component.

               WARNING: This inversion trick only will return the ABSOLUTE value, so it will have the
               incorrect sign when inverting negatives! This is fine here, though, since our Z coords
               are in the positive direction, starting at 0.
            */
            trans_pos1.w = shz_invf_fsrra(trans_pos1.w);
            // Perspective divide the X component and write it to the store queue
            vert1->x = trans_pos1.x * trans_pos1.w;
            // Perspective divide the Y component and write it to the stoer queue.
            vert1->y = trans_pos1.y * trans_pos1.w;
            // Use the W component directly as our transformed Z.
            vert1->z = trans_pos1.w;
            // Submit the vertex that we've filled the store queue with to the PVR's tile accelerator.
            pvr_dr_commit(vert1);

            // Compute the local position of the second vertex.
            shz_vec3_t local_pos2 = shz_vec3_scale(shz_vec3_init(sc_slice.cos * r1, sc_slice.sin * r1, z1), radius);

            // Transform the second local position by the XMTRX matrix.
            shz_vec4_t trans_pos2 = shz_xmtrx_transform_vec4(shz_vec3_vec4(local_pos2, 1.0f));
            // Since we applied a WXYZ swizzle to the projection matrix to get the W component first, we have to deswizzle.
            trans_pos2 = shz_vec4_swizzle(trans_pos2, 1, 2, 3, 0);

            // Map a PVR vertex into the second store queue, so we can fill it in.
            pvr_vertex_t* vert2 = pvr_dr_target();

            // Prevent GCC from reordering instructions beyond this boundary.
            SHZ_MEMORY_BARRIER_SOFT();

            // Use fast inversion approximation on W for perspective divison.
            trans_pos2.w = shz_invf_fsrra(trans_pos2.w);
            // Perspective divide the X component and write it to the store queue
            vert2->x = trans_pos2.x * trans_pos2.w;
            // Perspective divide the Y component and write it to the stoer queue.
            vert2->y = trans_pos2.y * trans_pos2.w;
            // Use the W component directly as our transformed Z.
            vert2->z = trans_pos2.w;
            // Submit the second filled-in vertex to the PVR's tile accelerator the the store queue.
            pvr_dr_commit(vert2);
        }
        {
            // Calculate the angle of rotation for the given slice.
            const float sliceAngle = SPHERE_SLICES * sliceStep;
            // Calculate the sin/cos pairs for the given angle.
            shz_sincos_t sc_slice = shz_sincosf(sliceAngle);
            // Calculate the local position for this vertex.
            shz_vec3_t local_pos1 = shz_vec3_scale(shz_vec3_init(sc_slice.cos * r2, sc_slice.sin * r2, z2), radius);
            // Transform local coords extended into 4D by our transform matrix.
            shz_vec4_t trans_pos1 = shz_xmtrx_transform_vec4(shz_vec3_vec4(local_pos1, 1.0f));
            // Swizzle our result position back to undo the permutation (WXYZ => XYZW).
            trans_pos1 = shz_vec4_swizzle(trans_pos1, 1, 2, 3, 0);

            // Map a vertex pointer into the store queues so we can begin filling it in.
            pvr_vertex_t* vert1 = pvr_dr_target();

            // Prevent GCC from reordering instructions beyond this boundary.
            SHZ_MEMORY_BARRIER_SOFT();

            // Calculate inverse W for perspective division by taking the fast inverse of W.
            trans_pos1.w = shz_invf_fsrra(trans_pos1.w);
            // Store X perspective divided by W into the store queue.
            vert1->x = trans_pos1.x * trans_pos1.w;
            // Store Y perspective divided by W into the store queue.
            vert1->y = trans_pos1.y * trans_pos1.w;
            // Store inverse W as Z.
            vert1->z = trans_pos1.w;
            // Submit this vertex to the PVR's TA now that we've filled it in.
            pvr_dr_commit(vert1);
           
            // Calculate local position for the next vertex.
            shz_vec3_t local_pos2 = shz_vec3_scale(shz_vec3_init(sc_slice.cos * r1, sc_slice.sin * r1, z2), radius);
            // Transform local coords extended into 4D by XMTRX.
            shz_vec4_t trans_pos2 = shz_xmtrx_transform_vec4(shz_vec3_vec4(local_pos2, 1.0f));
            // Deswizzle our transformed position, undoing our permutation matrix we applied to the perspective matrix.
            trans_pos2 = shz_vec4_swizzle(trans_pos2, 1, 2, 3, 0);
            
            // Map a pointer to a PVR vertex into a store queue.
            pvr_vertex_t* vert2 = pvr_dr_target();

            // Prevent GCC from reordering instructions beyond this boundary.
            SHZ_MEMORY_BARRIER_SOFT();

            // Signal that this is the last vertex in the triangle strip.
            vert2->flags = PVR_CMD_VERTEX_EOL;  
            // Calculate 1/W to be used for perspective division.
            trans_pos2.w = shz_invf_fsrra(trans_pos2.w);
            // Write X perspective divided by W into the SQ.
            vert2->x = trans_pos2.x * trans_pos2.w;
            // Write Y perspective divided by W into the SQ.
            vert2->y = trans_pos2.y * trans_pos2.w;
            // Write Z = 1/W into the SQ.
            vert2->z = trans_pos2.w;
            // Flush the current vertex to the TA.
            pvr_dr_commit(vert2);
        }
    }
}

/*! Initializes Bruce's balls.

    Precalculates the balls' attributes, aligning them to a grid and
    givign them intial orientation and velocity values.
*/
static void init_balls(Ball *balls) {
    // Precalculate initialization constants.
    constexpr unsigned GRID_COLS = 10;
    constexpr unsigned GRID_ROWS = 10;
    constexpr float    SPACING_X = 4.0f;
    constexpr float    SPACING_Y = 4.0f;
    constexpr float    START_X   = -((GRID_COLS - 1) * SPACING_X) / 2.0f;
    constexpr float    START_Y   = -((GRID_ROWS - 1) * SPACING_Y) / 2.0f;
    constexpr float    BASE_Z    = 35.0f;
    
    unsigned idx = 0;
    for(unsigned y = 0; y < GRID_ROWS; y++) {
        for(unsigned x = 0; x < GRID_COLS; x++) {
            if(SHZ_LIKELY(idx < BALL_COUNT)) {
                // Preallocate the cache line above the ball, since it perfectly fits into a cache line.
                shz_dcache_alloc_line(&balls[idx]);
                // Calculate the ball's position.
                balls[idx].pos = shz_vec3_init(START_X + (x * SPACING_X),
                                               START_Y + (y * SPACING_Y),
                                               BASE_Z);
                // Start each ball with no rotation.
                balls[idx].rot = shz_vec2_init(0.0f, 0.0f);
                // Give each ball an initial velocity.
                balls[idx].vel = shz_vec2_init(0.01f  + (idx % 5) * 0.002f,
                                               0.015f + (idx % 7) * 0.002f);
                // Give each ball an initial color.
                balls[idx].color = 0xff000000 
                                 | (((idx * 11) % 256) << 16)
                                 | (((idx *  7) % 256) <<  8) 
                                 | (((idx *  5) % 256) <<  0);
                idx++;
            }
        }
    }
}

//! Polls to see whether the start button of the fist controller was pressed.
static bool check_exit(void) {
    // Grab the first maple device which implements controller functionality.
    maple_device_t* cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    /* Check whether we found such a device.
    
        NOTE: SOMETIMES using SHZ_LIKELY() and SHZ_UNLIKELY() to tell the compiler
        about the likelihood of a condition being true results in better code
        generation, typically with inner contionals within a for loop... 
       
        We're only using it here for educational purposes. ;)
    */
    if(SHZ_LIKELY(cont)) {
        // Grab the status of the device, which contains its button states.
        cont_state_t* state = (cont_state_t *)maple_dev_status(cont);
        // Return true if the state exists and the start button is pressed.
        if(SHZ_UNLIKELY(state && state->start))
            return true;
    }

    // Return false if the start button on the first detected controller is not pressed.
    return false;
}

//! Calculates and intermittently reports frame statistics.
static void update_frame_stats(void) {
    //! Structure containing statistics we're measuring.
    static struct {
        uint64_t fps_timer; //!< Timestamp from the last time this routine was called.
        unsigned frames;    //!< The number of rames that have run within the last second.
        float    fps;       //!< The last calculated framerate.
    } stats;                //!< Static instance of statistics, which persists between frames.
 
    // Report that another frame has run.
    stats.frames++;
    // Grab the current timestamp using the highest precision timer.
    uint64_t current_time = timer_ns_gettime64();

    // Check whether we are running for the first time.
    if(SHZ_UNLIKELY(!stats.fps_timer)) {
        // Initialize the fps_timer for the first pass.
        stats.fps_timer = current_time; 
    // Check whether a second has ellapsed.
    } else if(current_time - stats.fps_timer >= 1000000000) {
        // Calculate how many frames were able to run within the last second.
        stats.fps = stats.frames / ((current_time - stats.fps_timer) / 1000000000.0f);
        // Calculate the number of polygons which were rendered within the last second.
        float pps = TRIANGLES_TOTAL * stats.fps;
        
        // Output statistics.
        printf("FPS: %.4f | PPS: %.4fM | Tris: %u\n", 
               stats.fps, pps / 1000000.0f, TRIANGLES_TOTAL);
        
        // Rest our stats for the next second.
        stats.frames = 0;
        stats.fps_timer = timer_ns_gettime64();
    }
}

//! Program entry point.
int main(int argc, const char *argv[]) {
    // Array of all balls within the scene, each mapping perfectly to a cache line.
    Ball balls[BALL_COUNT];
    // Over-aligned projection_view matrix, occupying two cache lines.
    alignas(32) shz_mat4x4_t projection_view;
    // Cached polygon header to be submitted when drawing Bruce's balls.
    pvr_poly_hdr_t poly_header;
    
    // Initialize the PVR.
    pvr_init(&(const pvr_init_params_t) {
        // The only tile bins we need are those for opaque geometry. This is 32 bins per TA tile.
        { PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_0, PVR_BINSIZE_0, PVR_BINSIZE_0 },
        // We need 3MB of vertex array storage in VRAM to fit all of Bruce's balls.
        1024 * 1024 * 3,
        // No vertex DMA
        0,
        // No FSAA
        0,
        // Enable translucent polygon autosort
        0,
        // Preallocate 6 extra OPBs
        6,
        // Default vertex buffer double-buffering
        0
    });

    // Precompute our projection view matrix.
    setup_projection_view(&projection_view);
    // Precache our polygon header.
    cache_polygon_header(&poly_header);
    
    // Set up Bruce's balls.
    init_balls(balls);
    
    // Print initial metrics.
    printf("======== Lets play with Bruce's Balls!!! ========\n");
    printf("Rendering %u balls, %u triangles\n", BALL_COUNT, TRIANGLES_TOTAL);
    printf("=================================================\n");

    // Loop forever... unless the start button is pressed.
    while(SHZ_LIKELY(!check_exit())) {
        // Begin rendering a scene with the PVR GPU.
        pvr_scene_begin();
        // Begin submitting opaque geometry to the PVR.
        pvr_list_begin(PVR_LIST_OP_POLY);

        // Prefetch Bruce's first ball.
        SHZ_PREFETCH(&balls[0]);

        // Submit our polygon header used by all of Bruce's balls.
        submit_polygon_header(&poly_header);

        // Render Bruce's balls.
        for(unsigned i = 0; i < BALL_COUNT; i++) {
            // Prefetch the projection view matrix into the cache.
            SHZ_PREFETCH(&projection_view);
            // Update the ball's orientation from its angular velocity.
            balls[i].rot = shz_vec2_add(balls[i].rot, balls[i].vel);
            // Apply the final transform matrix for the current ball.
            apply_model_matrix(balls[i].pos, balls[i].rot, &projection_view);
            // Prefetch Bruce's next ball while we're drawing his current one.
            SHZ_PREFETCH(&balls[i + 1]);
            //! Render Bruce's current ball.
            render_sphere(1.0f, balls[i].color);
        }
        
        // Finish submitting geometry to the opaque geometry list.
        pvr_list_finish();
        // Finish rendering the current scene.
        pvr_scene_finish();
        // Update our stats for this frame.
        update_frame_stats();
    }
    
    // Shut down the GPU before we exit.
    pvr_shutdown();

    // Return a success status code.
    return EXIT_SUCCESS;
}
