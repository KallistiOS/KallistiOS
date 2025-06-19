/* KallistiOS ##version##

   dc/pvr/pvr_header.h
   Copyright (C) 2025 Paul Cercueil
*/

/** \file       dc/pvr/pvr_header.h
    \brief      Polygon/Sprite header definitions
    \ingroup    pvr

  \author Paul Cercueil
*/

#ifndef __DC_PVR_PVR_HEADER_H
#define __DC_PVR_PVR_HEADER_H

#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \defgroup pvr_primitives_headers Headers
    \brief                           Structs relative to PVR headers
    \ingroup pvr_primitives
    \headerfile dc/pvr/pvr_header.h

    @{
*/

/** \brief   Vertex color formats

    These control how colors are represented in polygon data.
*/
typedef enum pvr_color_fmts {
    PVR_CLRFMT_ARGBPACKED,      /**< 32-bit integer ARGB */
    PVR_CLRFMT_4FLOATS,         /**< 4 floating point values */
    PVR_CLRFMT_INTENSITY,       /**< Intensity color */
    PVR_CLRFMT_INTENSITY_PREV,  /**< Use last intensity */
} pvr_color_fmts_t;

/** \brief   Primitive clipping modes

    These control how primitives are clipped against the user clipping area.
*/
typedef enum pvr_clip_mode {
    PVR_USERCLIP_DISABLE = 0,   /**< Disable clipping */
    PVR_USERCLIP_INSIDE = 2,    /**< Enable clipping inside area */
    PVR_USERCLIP_OUTSIDE = 3,   /**< Enable clipping outside area */
} pvr_clip_mode_t;

/** \brief   PVR rendering lists

    Each primitive submitted to the PVR must be placed in one of these lists,
    depending on its characteristics.
*/
typedef enum pvr_list_type {
    PVR_LIST_OP_POLY,           /**< Opaque polygon list */
    PVR_LIST_OP_MOD,            /**< Opaque modifier list */
    PVR_LIST_TR_POLY,           /**< Translucent polygon list */
    PVR_LIST_TR_MOD,            /**< Translucent modifier list*/
    PVR_LIST_PT_POLY,           /**< Punch-thru polygon list */
} pvr_list_type_t;

/** \brief   Primitive culling modes

    These culling modes can be set by polygons to determine when they are
    culled. They work pretty much as you'd expect them to if you've ever used
    any 3D hardware before.
*/
typedef enum pvr_cull_mode {
    PVR_CULLING_NONE,           /**< Disable culling */
    PVR_CULLING_SMALL,          /**< Cull if small */
    PVR_CULLING_CCW,            /**< Cull if counterclockwise */
    PVR_CULLING_CW,             /**< Cull if clockwise */
} pvr_cull_mode_t;

/** \brief   Depth comparison modes

    These set the depth function used for comparisons.
*/
typedef enum pvr_depthcmp_mode {
    PVR_DEPTHCMP_NEVER,         /**< Never pass */
    PVR_DEPTHCMP_LESS,          /**< Less than */
    PVR_DEPTHCMP_EQUAL,         /**< Equal to */
    PVR_DEPTHCMP_LEQUAL,        /**< Less than or equal to */
    PVR_DEPTHCMP_GREATER,       /**< Greater than */
    PVR_DEPTHCMP_NOTEQUAL,      /**< Not equal to */
    PVR_DEPTHCMP_GEQUAL,        /**< Greater than or equal to */
    PVR_DEPTHCMP_ALWAYS,        /**< Always pass */
} pvr_depthcmp_mode_t;

/** \brief   Texture U/V size */
typedef enum pvr_uv_size {
    PVR_UV_SIZE_8,
    PVR_UV_SIZE_16,
    PVR_UV_SIZE_32,
    PVR_UV_SIZE_64,
    PVR_UV_SIZE_128,
    PVR_UV_SIZE_256,
    PVR_UV_SIZE_512,
    PVR_UV_SIZE_1024,
} pvr_uv_size_t;

/** \brief   Texture color calculation modes.

    The shading mode specifies how the pixel value used as the "foreground" or
    "source" for blending is computed.

    Here, "tex" represents the pixel value from the texture, and "col"
    represents the pixel value from the polygon's color.  RGB() represents the
    color channels, A() represents the alpha channel, and ARGB() represents the
    whole pixel (color + alpha).

    Note that the offset color (aka. oargb), if specular lighting is enabled,
    is added to the result. Its alpha channel is ignored.
*/
typedef enum pvr_txr_shading_mode {
    PVR_TXRENV_REPLACE,         /**< px = ARGB(tex) */
    PVR_TXRENV_MODULATE,        /**< px = A(tex) + RGB(col) * RGB(tex) */
    PVR_TXRENV_DECAL,           /**< px = A(col) + RGB(tex) * A(tex) + RGB(col) * (1 - A(tex)) */
    PVR_TXRENV_MODULATEALPHA,   /**< px = ARGB(col) * ARGB(tex) */
} pvr_txr_shading_mode_t;

/** \brief   Texture sampling modes */
typedef enum pvr_filter_mode {
    PVR_FILTER_NEAREST,         /**< No filtering (point sample) */
    PVR_FILTER_BILINEAR,        /**< Bilinear interpolation */
    PVR_FILTER_TRILINEAR1,      /**< Trilinear interpolation pass 1 */
    PVR_FILTER_TRILINEAR2,      /**< Trilinear interpolation pass 2 */
    PVR_FILTER_NONE = PVR_FILTER_NEAREST,
} pvr_filter_mode_t;

/** \brief   Fog modes

    Each polygon can decide what fog type is used by specifying the fog mode
    in its header.
*/
typedef enum pvr_fog_type {
    PVR_FOG_TABLE,              /**< Table fog */
    PVR_FOG_VERTEX,             /**< Vertex fog */
    PVR_FOG_DISABLE,            /**< Disable fog */
    PVR_FOG_TABLE2,             /**< Table fog mode 2 */
} pvr_fog_type_t;

/** \brief   Blending modes

    These are all the blending modes that can be done with regard to alpha
    blending on the PVR.
*/
typedef enum pvr_blend_mode {
    PVR_BLEND_ZERO,             /**< None of this color */
    PVR_BLEND_ONE,              /**< All of this color */
    PVR_BLEND_DESTCOLOR,        /**< Destination color */
    PVR_BLEND_INVDESTCOLOR,     /**< Inverse of destination color */
    PVR_BLEND_SRCALPHA,         /**< Blend with source alpha */
    PVR_BLEND_INVSRCALPHA,      /**< Blend with inverse source alpha */
    PVR_BLEND_DESTALPHA,        /**< Blend with destination alpha */
    PVR_BLEND_INVDESTALPHA,     /**< Blend with inverse destination alpha */
} pvr_blend_mode_t;

/** \brief   Texture formats

    These are the texture formats that the PVR supports.
*/
typedef enum pvr_pixel_mode {
    PVR_PIXEL_MODE_ARGB1555,    /**< 16-bit ARGB1555 */
    PVR_PIXEL_MODE_RGB565,      /**< 16-bit RGB565 */
    PVR_PIXEL_MODE_ARGB4444,    /**< 16-bit ARGB4444 */
    PVR_PIXEL_MODE_YUV422,      /**< YUV422 format */
    PVR_PIXEL_MODE_BUMP,        /**< Bumpmap format */
    PVR_PIXEL_MODE_PAL_4BPP,    /**< 4BPP paletted format */
    PVR_PIXEL_MODE_PAL_8BPP,    /**< 8BPP paletted format */
} pvr_pixel_mode_t;

/** \brief   Triangle strip length

    This sets the maximum length of a triangle strip, if not
    configured in auto mode.
*/
typedef enum pvr_strip_len {
    PVR_STRIP_LEN_1,
    PVR_STRIP_LEN_2,
    PVR_STRIP_LEN_4,
    PVR_STRIP_LEN_6,
} pvr_strip_len_t;

/** \brief   Polygon header type

    This enum contains the possible PVR header types.
*/
typedef enum pvr_hdr_type {
    PVR_HDR_EOL = 0,
    PVR_HDR_USERCLIP = 1,
    PVR_HDR_OBJECT_LIST_SET = 2,
    PVR_HDR_POLY = 4,
    PVR_HDR_SPRITE = 5,
} pvr_hdr_type_t;

/** \brief   Texture address

    This type represents an address of a texture in VRAM,
    pre-processed to be used in headers.
*/
typedef uint32_t pvr_txr_ptr_t;

/** \brief   Get texture address from VRAM address

    This function can be used to get a texture address that can be used
    in a PVR header from the texture's VRAM address.

    \param  addr            The texture's address in VRAM
    \return                 The pre-processed texture address
*/
static inline pvr_txr_ptr_t to_pvr_txr_ptr(pvr_ptr_t addr) {
    return ((uint32_t)addr & 0x00fffff8) >> 3;
}

/** \brief Get texture address form VRAM address

    Alias macro for to_pvr_txr_ptr().
*/
#define pvr_to_pvr_txr_ptr(addr) to_pvr_txr_ptr(addr)

/** \brief   PVR header command

    This structure contains all the fields for the command of PVR headers.
*/
typedef struct pvr_poly_hdr_cmd {
    bool uvfmt_f16               :1; /* 0 */     /**< Use 16-bit floating-point U/Vs */
    bool gouraud                 :1; /* 1 */     /**< Enable gouraud shading */
    bool oargb_en                :1; /* 2 */     /**< Enable specular lighting */
    bool txr_en                  :1; /* 3 */     /**< Enable texturing */
    pvr_color_fmts_t color_fmt   :2; /* 5-4 */   /**< Select color encoding */
    bool mod_normal              :1; /* 6 */     /**< true: normal, false: cheap shadow */
    bool modifier_en             :1; /* 7 */     /**< Enable modifier effects */
    uint32_t                     :8; /* 15-8 */
    pvr_clip_mode_t clip_mode    :2; /* 17-16 */ /**< Clipping mode */
    pvr_strip_len_t strip_len    :2; /* 19-18 */ /**< Triangle strips length (if non-auto) */
    uint32_t                     :3; /* 22-20 */
    bool auto_strip_len          :1; /* 23 */    /**< Auto select triangle strips length */
    pvr_list_type_t list_type    :3; /* 26-24 */ /**< Render list to use */
    uint32_t                     :1; /* 27 */
    bool strip_end               :1; /* 28 */    /**< Mark an end-of-strip */
    pvr_hdr_type_t hdr_type      :3; /* 31-29 */ /**< Header type */
} pvr_poly_hdr_cmd_t;

/** \brief   PVR header mode1

    This structure contains all the fields for the mode1 parameter of
    PVR headers.
*/
typedef struct pvr_poly_hdr_mode1 {
    uint32_t                      :25; /* 24-0 */
    bool txr_en                   :1; /* 25 */    /**< Enable texturing (2nd bit) */
    bool depth_write_dis          :1; /* 26 */    /**< Disable depth writes */
    pvr_cull_mode_t culling       :2; /* 28-27 */ /**< Culling mode */
    pvr_depthcmp_mode_t depth_cmp :3; /* 31-29 */ /**< Depth comparison mode */
} pvr_poly_hdr_mode1_t;

/** \brief   PVR header mode2

    This structure contains all the fields for the mode2 parameter of
    PVR headers.
*/
typedef struct pvr_poly_hdr_mode2 {
    pvr_uv_size_t v_size           :3; /* 2-0 */   /**< Texture height */
    pvr_uv_size_t u_size           :3; /* 5-3 */   /**< Texture width */
    pvr_txr_shading_mode_t shading :2; /* 7-6 */   /**< Shading mode */
    uint32_t mip_bias              :4; /* 11-8 */  /**< Bias for mipmaps */
    bool supersampling             :1; /* 12 */    /**< Enable texture supersampling */
    pvr_filter_mode_t filter_mode  :2; /* 14-13 */ /**< Texture filtering mode */
    bool v_clamp                   :1; /* 15 */    /**< Clamp V to 1.0 */
    bool u_clamp                   :1; /* 16 */    /**< Clamp U to 1.0 */
    bool v_flip                    :1; /* 17 */    /**< Flip V after 1.0 */
    bool u_flip                    :1; /* 18 */    /**< Flip U after 1.0 */
    bool txralpha_dis              :1; /* 19 */    /**< Disable alpha channel in textures */
    bool alpha                     :1; /* 20 */    /**< Enable alpha channel in vertex colors */
    bool fog_clamp                 :1; /* 21 */    /**< Enable fog clamping */
    pvr_fog_type_t fog_type        :2; /* 23-22 */ /**< Select fog type */
    bool blend_dst_acc2            :1; /* 24 */    /**< Blend to the 2nd accumulation buffer */
    bool blend_src_acc2            :1; /* 25 */    /**< Blend from the 2nd accumulation buffer */
    pvr_blend_mode_t blend_dst     :3; /* 28-26 */ /**< Blend mode for the background */
    pvr_blend_mode_t blend_src     :3; /* 31-29 */ /**< Blend mode for the foreground */
} pvr_poly_hdr_mode2_t;

/** \brief   PVR header mode3

    This structure contains all the fields for the mode3 parameter of
    PVR headers.
*/
typedef struct pvr_poly_hdr_mode3 {
    pvr_txr_ptr_t txr_base       :25; /* 24-0 */ /**< Pre-processed texture address */
    bool x32stride               :1; /* 25 */    /**< Set if texture stride is multiple of 32 */
    bool nontwiddled             :1; /* 26 */    /**< Set if texture is not twiddled */
    pvr_pixel_mode_t pixel_mode  :3; /* 29-27 */ /**< Select the texture's pixel format */
    bool vq_en                   :1; /* 30 */    /**< Set if the texture is VQ encoded */
    bool mipmap_en               :1; /* 31 */    /**< Enable mipmaps */
} pvr_poly_hdr_mode3_t;

/** \brief   PVR polygon header.

    This structure contains information about how the following polygons should
    be rendered.
*/
typedef __attribute__((aligned(32))) struct pvr_poly_hdr {
    union {
        uint32_t cmd;                              /**< Raw access to cmd param */
        pvr_poly_hdr_cmd_t m0;                     /**< command parameters */
    };
    union {
        uint32_t mode1;                            /**< Raw access to mode1 param */
        pvr_poly_hdr_mode1_t m1;                   /**< mode1 parameters */
    };
    union {
        uint32_t mode2;                            /**< Raw access to mode2 param */
        uint32_t mode2_0;                          /**< Legacy name */
        pvr_poly_hdr_mode2_t m2;                   /**< mode2 parameters (modifiers: outside volume) */
    };
    union {
        uint32_t mode3;                            /**< Raw access to mode3 param */
        uint32_t mode3_0;                          /**< Legacy name */
        pvr_poly_hdr_mode3_t m3;                   /**< mode3 parameters (modifiers: outside volume) */
    };
    union {
        struct {
            /* Intensity color */
            float a;                               /**< Intensity color alpha */
            float r;                               /**< Intensity color red */
            float g;                               /**< Intensity color green */
            float b;                               /**< Intensity color blue */
        };
        struct {
            /* Modifier volume */
            union {
                struct {
                    uint32_t mode2_1;              /**< Legacy name */
                    uint32_t mode3_1;              /**< Legacy name */
                };
                struct {
                    pvr_poly_hdr_mode2_t m2;       /**< mode2 parameters (modifiers: inside volume) */
                    pvr_poly_hdr_mode3_t m3;       /**< mode3 parameters (modifiers: inside volume) */
                } modifier;                        /**< Modifier volume parameters */
            };
        };
        struct {
            /* Sprite */
            uint32_t argb;                         /**< 32-bit ARGB vertex color for sprites */
            uint32_t oargb;                        /**< 32-bit ARGB specular color for sprites */
        };
        struct {
            /* User clip area */
            uint32_t start_x;                      /**< Left (inclusive) border of user clip area */
            uint32_t start_y;                      /**< Top (inclusive) border of user clip area */
            uint32_t end_x;                        /**< Right (inclusive) border of user clip area */
            uint32_t end_y;                        /**< Bottom (inclusive) border of user clip area */
        };
    };
} pvr_poly_hdr_t;

_Static_assert(sizeof(pvr_poly_hdr_t) == 32, "Invalid header size");

/** @} */

__END_DECLS

#endif /* __DC_PVR_PVR_HEADER_H */
