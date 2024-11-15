/* KallistiOS ##version##

   dc/pvr.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2014 Lawrence Sebald
   Copyright (C) 2023 Ruslan Rostovtsev
   Copyright (C) 2024 Falco Girgis

   Low-level PVR 3D interface for the DC
*/

/** \file    dc/pvr.h
    \brief   Low-level PVR (3D hardware) interface.
    \ingroup pvr

    This file provides support for using the PVR 3D hardware in the Dreamcast.
    Note that this does not handle any sort of perspective transformations or
    anything of the like. This is just a very thin wrapper around the actual
    hardware support.

    \remark
    This file is used for pretty much everything related to the PVR, from memory
    management to actual primitive rendering.

    \todo 
        - Auxiliary accumulation buffer example/documentation
        - RTT texture sizes
        - Multipass rendering
        - pvr_poly_ctx_t::specular2
            - specular enabled within modifier?
        - Missing fog mode
        - Global "Intensity" light mode?
        - PVR_MIPBIAS_0 
            - Does it exist? What happens if set to 0?
        - pvr_list_flush() implementation
        - pvr_vertbuf_written()
            - shouldn't this at least assert if you write to much? Error code?
        - pvr_vertbuf_remaining()
            - query for the amount of storage remaining in a vertex buffer?

    \author Megan Potter
    \author Roger Cattermole
    \author Paul Boese
    \author Brian Paul
    \author Lawrence Sebald
    \author Benoit Miller
    \author Ruslan Rostovtsev
    \author Falco Girgis
*/

#ifndef __DC_PVR_H
#define __DC_PVR_H

#include <stdint.h>
#include <stdbool.h>

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdalign.h>

#include <arch/memory.h>
#include <arch/types.h>
#include <arch/cache.h>
#include <dc/sq.h>

/** \defgroup pvr   PowerVR
    \brief          Low-level PowerVR GPU Driver.
    \ingroup        video
*/

/** PVR Primitive List Type
    \ingroup pvr_list_mgmt
    
    This type is an identifier for the different kinds of primitive lists.

    \note
    Each primitive processed by the PVR is submitted to one of the hardware
    primitive lists. There is a separate list for each transparency type for
    both primitives (polygons or sprites) and modifier volumes. 
*/
typedef enum pvr_list {
    PVR_LIST_OP_POLY, /**< Opaque polygon list */
    PVR_LIST_OP_MOD,  /**< Opaque modifier list */
    PVR_LIST_TR_POLY, /**< Translucent polygon list */
    PVR_LIST_TR_MOD,  /**< Translucent modifier list*/
    PVR_LIST_PT_POLY, /**< Punch-thru polygon list */
    PVR_LIST_COUNT    /**< Number of list types */
} pvr_list_t;

#include "pvr/pvr_regs.h"
#include "pvr/pvr_mem.h"
#include "pvr/pvr_misc.h"
#include "pvr/pvr_prim.h"
#include "pvr/pvr_palette.h"
#include "pvr/pvr_context.h"
#include "pvr/pvr_dma.h"
#include "pvr/pvr_texture.h"
#include "pvr/pvr_fog.h"
#include "pvr/pvr_palette.h"
#include "pvr/pvr_legacy.h"

/** \defgroup pvr_init  Initialization
    \brief    PowerVR Initialization and Shutdown
    \ingroup  pvr
    @{
*/

/** PVR Primitive Bin Sizes */
typedef enum pvr_bin_size {
    PVR_BINSIZE_0  = 0,   /**< 0-length (disables the list) */
    PVR_BINSIZE_8  = 8,   /**< 8-word (32-byte) length */
    PVR_BINSIZE_16 = 16,  /**< 16-word (64-byte) length */
    PVR_BINSIZE_32 = 32   /**< 32-word (128-byte) length */
} pvr_bin_size_t;

/** PVR initialization structure

    This structure defines how the PVR initializes various parts of the system,
    including the primitive bin sizes, the vertex buffer size, and whether
    vertex DMA will be enabled.

    You essentially fill one of these in, and pass it to pvr_init().
*/
typedef struct pvr_init_params {
    /** Primitve List Bin sizes.

        The bins go in the following order: opaque polygons, opaque modifiers,
        translucent polygons, translucent modifiers, punch-thrus
    */
    pvr_bin_size_t opb_sizes[PVR_LIST_COUNT];

    /** Vertex buffer size 

        \warning
        This value should be a nice, round number, such as being in KB, or
        evenly divisible by `1024`.
    */
    size_t        vertex_buf_size;

    /** Enable vertex DMA?

        Set to non-zero if we want to enable vertex DMA mode. Note that if this
        is set, then _all_ enabled lists need to have a vertex buffer assigned,
        even if you never use that list for anything.
    */
    bool          dma_enabled;

    /** Enable horizontal scaling?

        Set to non-zero if horizontal scaling is to be enabled. By enabling this
        setting and stretching your image to double the native screen width, you
        can get horizontal full-screen anti-aliasing. 
    */
    bool          fsaa_enabled;

    /** Disable translucent polygon autosort?

        Set to non-zero to disable translucent polygon autosorting. By enabling
        this setting, the PVR acts more like a traditional Z-buffered system
        when rendering translucent polygons, meaning you must pre-sort them
        yourself if you want them to appear in the right order. 
    */
    bool          autosort_disabled;

    /** OPB Overflow Count.

        Preallocates this many extra OPBs (sets of tile bins), allowing the PVR
        to use the extra space when there's too much geometry in the first OPB.
    
        Increasing this value can eliminate artifacts where pieces of geometry
        flicker in and out of existence along the tile boundaries. 
    */
    size_t        opb_overflow_count;
} pvr_init_params_t;

/** Initialize the PVR chip to ready status.
    
    This function enables the specified lists and uses the specified parameters.
    Note that bins and vertex buffers come from the texture memory pool, so only
    allocate what you actually need. Expects that a 2D mode was initialized
    already using the vid_* API.

    \param  params          The set of parameters to initialize with
    \retval 0               On success
    \retval -1              If the PVR has already been initialized or the video
                            mode active is not suitable for 3D

    \sa pvr_init_defaults(), pvr_shutdown()
*/
int pvr_init(const pvr_init_params_t *params);

/** Simple PVR initialization.

    This simpler function initializes the PVR using 16/16 for the opaque
    and translucent lists' bin sizes, and 0's for everything else. It sets 512KB
    of vertex buffer. This is equivalent to the old ta_init_defaults() for now.

    \retval 0               On success
    \retval -1              If the PVR has already been initialized or the video
                            mode active is not suitable for 3D

    \sa pvr_init(), pvr_shutdown()
*/
int pvr_init_defaults(void);

/** Shut down the PVR chip from ready status.

    This essentially leaves the video system in 2D mode as it was before the
    init.

    \retval 0               On success
    \retval -1              If the PVR has not been initialized

    \sa pvr_init(), pvr_init_defaults()
*/
int pvr_shutdown(void);

/** @} */

/** \defgroup   pvr_scene_mgmt  Scene Submission
    \brief                      PowerVR API for submitting scene geometry
    \ingroup                    pvr

    This API is used to submit triangle strips to the PVR via the TA
    interface in the chip.

    An important side note about the PVR is that all primitive types
    must be submitted grouped together. If you have 10 polygons for each
    list type, then the PVR must receive them via the TA by list type,
    with a list delimiter in between.

    So there are two modes you can use here. The first mode allows you to
    submit data directly to the TA. Your data will be forwarded to the
    chip for processing as it is fed to the PVR module. If your data
    is easily sorted into the primitive types, then this is the fastest
    mode for submitting data.

    The second mode allows you to submit data via main-RAM vertex buffers,
    which will be queued until the proper primitive type is active. In this
    case, each piece of data is copied into the vertex buffer while the
    wrong list is activated, and when the proper list becomes activated,
    the data is all sent at once. Ideally this would be via DMA, right
    now it is by store queues. This has the advantage of allowing you to
    send data in any order and have the PVR functions resolve how it should
    get sent to the hardware, but it is slower.

    The nice thing is that any combination of these modes can be used. You
    can assign a vertex buffer for any list, and it will be used to hold the
    incoming vertex data until the proper list has come up. Or if the proper
    list is already up, the data will be submitted directly. So if most of
    your polygons are opaque, and you only have a couple of translucents,
    you can set a small buffer to gather translucent data and then it will
    get sent when you do a pvr_end_scene().

    \remark
    Thanks to Mikael Kalms for the idea for this API.

    \note
    Another somewhat subtle point that bears mentioning is that in the normal
    case (interrupts enabled) an interrupt handler will automatically take
    care of starting a frame rendering (after scene_finish()) and also
    flipping pages when appropriate. 

    @{
*/

/** \defgroup  pvr_vertex_dma   Vertex DMA
    \brief     Use the DMA to transfer inactive lists to the PVR

    @{
*/

/** Is PVR vertex DMA enabled?
    
    \return `true` if vertex DMA was enabled at init time, `false` otherwise
*/
bool pvr_vertex_dma_enabled(void);

/** Setup a vertex buffer for one of the list types.

    If the specified list type already has a vertex buffer, it will be replaced
    by the new one. 

    \note
    Each buffer should actually be twice as long as what you will need to hold
    two frames worth of data).

    \warning
    You should generally not try to do this at any time besides before a frame
    is begun, or Bad Things May Happen.

    \param  list            The primitive list to set the buffer for.
    \param  buffer          The location of the buffer in main RAM. This must be
                            aligned to a 32-byte boundary.
    \param  len             The length of the buffer. This must be a multiple of
                            64, and must be at least 128 (even if you're not
                            using the list).
    
    \return                 The old buffer location (if any)

    \sa pvr_set_vertbuf(), pvr_vertbuf_written()
*/
void *pvr_set_vertbuf(pvr_list_t list, void *buffer, size_t len);

/** Retrieve a pointer to the current output location in the DMA buffer
    for the requested list.

    Vertex DMA must globally be enabled for this to work. Data may be added to
    this buffer by the user program directly; however, make sure to call
    pvr_vertbuf_written() to notify the system of any such changes.

    \param  list            The primitive list to get the buffer for.
    
    \return                 The tail of that list's buffer.

    \sa pvr_vertbuf_written(), pvr_set_vertbuf()
*/
void *pvr_vertbuf_tail(pvr_list_t list);

/** Notify the PVR system that data have been written into the output
    buffer for the given list.

    This should always be done after writing data directly to these buffers or
    it will get overwritten by other data.

    \param  list            The primitive list that was modified.
    \param  amt             Number of bytes written. Must be a multiple of 32.

    \sa pvr_vertbuf_tail(), pvr_set_verbuf()
*/
void pvr_vertbuf_written(pvr_list_t list, size_t amt);

/** @} */

/** Begin data submission for a frame of 3D output to the off-screen frame
    buffer.

    You must call this function (or pvr_scene_begin_txr()) for ever frame of
    output.

    \sa pvr_scene_begin_txr(), pvr_scene_end()
*/
void pvr_scene_begin(void);

/** Begin collecting data for a frame of 3D output to the specified texture.

    This function currently only supports outputting at the same size as the
    actual screen. Thus, make sure rx and ry are at least large enough for that.
    For a 640x480 output, rx will generally be 1024 on input and ry 512, as
    these are the smallest values that are powers of two and will hold the full
    screen sized output.

    \param  txr             The texture to render to.
    \param  rx              Width of the texture buffer (in pixels).
    \param  ry              Height of the texture buffer (in pixels).

    \sa pvr_scene_begin(), pvr_scene_end()
*/
void pvr_scene_begin_txr(pvr_ptr_t txr, size_t *rx, size_t *ry);

/** \defgroup pvr_list_mgmt Polygon Lists
    \brief                  PVR API for managing list submission
    \ingroup                pvr_scene_mgmt

    @{
*/

/** Begin collecting data for the given list type.
 
    Lists do not have to be submitted in any particular order, but all types of
    a list must be submitted at once (unless vertex DMA mode is enabled).

    Note that there is no need to call this function in DMA mode unless you want
    to make use of pvr_prim() for compatibility. This function will
    automatically call pvr_list_finish() if a list is already opened before
    opening the new list.

    \param  list            The list to open.
    \retval 0               On success.
    \retval -1              If the specified list has already been closed.

    \sa pvr_list_finish()
*/
int pvr_list_begin(pvr_list_t list);            

/** End collecting data for the current list type.

    Lists can never be opened again within a single frame once they have been
    closed. Thus submitting a primitive that belongs in a closed list is
    considered an error. Closing a list that is already closed is also an error.

    Note that if you open a list but do not submit any primitives, a blank one
    will be submitted to satisfy the hardware. If vertex DMA mode is enabled,
    then this simply sets the current list pointer to no list, and none of the
    above restrictions apply.

    \retval 0               On success.
    \retval -1              On error.

    \sa pvr_list_begin()
*/
int pvr_list_finish(void);

/** Submit a primitive of the current list type.

    Note that any values submitted in this fashion will go directly to the
    hardware without any sort of buffering, and submitting a primitive of the
    wrong type will quite likely ruin your scene. Note that this also will not
    work if you haven't begun any list types (i.e., all data is queued). If DMA
    is enabled, the primitive will be appended to the end of the currently
    selected list's buffer.

    \note
    Calls pvr_sq_load() internally when PVR DMA is disabled or pvr_list_prim()
    internally when PVR DMA is enabled.
    
    \warning
    \p data must be 32-byte aligned!

    \param  data            The primitive to submit.
    \param  size            The length of the primitive, in bytes. Must be a
                            multiple of 32.
    
    \retval 0               On success.
    \retval -1              On error.
*/
int pvr_prim(const void *data, size_t size);

/** @} */

/** \defgroup pvr_direct  Direct Rendering
    \brief                API for using direct rendering with the PVR
    \ingroup              pvr_scene_mgmt

    @{
*/

/** Direct Rendering state variable type. */
typedef uintptr_t pvr_dr_state_t;

/** Initialize a state variable for Direct Rendering.

    Store Queues are used.

    \param  vtx_buf_ptr     A variable of type pvr_dr_state_t to init.

    \sa pvr_dr_finish()
*/
void pvr_dr_init(pvr_dr_state_t *vtx_buf_ptr);

/** Obtain the target address for Direct Rendering.

    \param  vtx_buf_ptr     State variable for Direct Rendering. Should be of
                            type pvr_dr_state_t, and must have been initialized
                            previously in the scene with pvr_dr_init().
    
    \return                 A write-only destination address where a primitive
                            should be written to get ready to submit it to the
                            TA in DR mode.
*/
#define pvr_dr_target(vtx_buf_ptr) \
    ({ \
        (vtx_buf_ptr) ^= 32; \
        (void *)(MEM_AREA_SQ_BASE | (vtx_buf_ptr)); \
    })

/** Commit a primitive written into the Direct Rendering target address.

    \param  addr            The address returned by pvr_dr_target(), after you
                            have written the primitive to it.
*/
#define pvr_dr_commit(addr) sq_flush(addr)

/** Finish work with Direct Rendering.

    Called atomatically in pvr_scene_finish().
    Use it manually if you want to release Store Queues earlier.

    \sa pvr_dr_init()
*/
void pvr_dr_finish(void);

/** \brief  Upload a 32-byte payload to the Tile Accelerator

    Upload the given payload to the Tile Accelerator. The difference with the
    Direct Rendering approach above is that the Store Queues are not used, and
    therefore can be used for anything else.

    \param  data            A pointer to the 32-byte payload.
                            The pointer must be aligned to 8 bytes.
*/
void pvr_send_to_ta(const void *data);

/** @} */

/** Submit a primitive of the given list type.

    Data will be queued in a vertex buffer, thus one must be available for the
    list specified (will be asserted by the code).

    \param  list            The list to submit to.
    \param  data            The primitive to submit.
    \param  size            The size of the primitive in bytes. This must be a
                            multiple of 32.
    
    \retval 0               On success.
    \retval -1              On error.
*/
int pvr_list_prim(pvr_list_t list, const void *data, size_t size);

/** Flush the buffered data of the given list type to the TA.

    This function is currently not implemented, and calling it will result in an
    assertion failure. It is intended to be used later in a "hybrid" mode where
    both direct and DMA TA submission is possible.

    \param  list            The list to flush.
    
    \retval -1              On error (it is not possible to succeed).
*/
int pvr_list_flush(pvr_list_t list);

/** Call this after you have finished submitting all data for a frame.

    Once this has been called, you can not submit any more data until one of the
    pvr_scene_begin() or pvr_scene_begin_txr() functions is called again.

    \retval 0               On success.
    \retval -1              On error (no scene started).

    \sa pvr_scene_begin(), pvr_scene_begin_txr()
*/
int pvr_scene_finish(void);

/** Block until the PVR is ready for the next frame.

    The PVR system allocates enough space for two frames: one in data collection
    mode, and another in rendering mode. If a frame is currently rendering, and
    another frame has already been closed, then the caller cannot do anything
    else until the rendering frame completes. Note also that the new frame
    cannot be activated except during a vertical blanking period, so this
    essentially waits until a rendered frame is complete and a vertical blank
    happens.

    \retval 0               On success. A new scene can be started now.
    \retval -1              On error. Something is probably very wrong...

    \sa pvr_is_ready()
*/
int pvr_wait_ready(void);

/** Check if the PVR is ready for the next frame.

    \deprecated
    Bad return value semantics (not ready is not actually an error); 
    replaced by pvr_is_ready().
    
    \retval 0               If the PVR is ready for a new scene. You must call
                            pvr_wait_ready() afterwards, before starting a new
                            scene.
    \retval -1              If the PVR is not ready for a new scene yet.

    \sa pvr_is_ready()
*/
int pvr_check_ready(void);

/** Check if the PVR is ready for the next frame.

    \note
    This function is equivalent to pvr_check_ready() with a sane return value.

    \retval false   The PVR is not yet raedy for another scene. You must call
                    pvr_wait_ready() afterwards, before starting a new scene.
    \retval true    The PVR is ready to begin a new scene.
 
    \sa pvr_wait_ready()
*/
static inline bool pvr_is_ready(void) {
    return !pvr_check_ready();
}

/** @} */

__END_DECLS

#endif
