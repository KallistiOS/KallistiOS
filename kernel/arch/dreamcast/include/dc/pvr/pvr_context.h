/* KallistiOS ##version##

   dc/pvr/pvr_context.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2014 Lawrence Sebald
   Copyright (C) 2023 Ruslan Rostovtsev
   Copyright (C) 2024 Falco Girgis
*/

/** \file       dc/pvr/pvr_context.h
    \brief      API for creating and compiling PowerVR primitive contexts
    \ingroup    pvr_vram

    \author Megan Potter
    \author Roger Cattermole
    \author Paul Boese
    \author Brian Paul
    \author Lawrence Sebald
    \author Benoit Miller
    \author Ruslan Rostovtsev
    \author Falco Girgis
*/

#ifndef __DC_PVR_PVR_CONTEXT_H
#define __DC_PVR_PVR_CONTEXT_H

#include <stdint.h>

#include <sys/cdefs.h>
__BEGIN_DECLS


/** \defgroup pvr_ctx Contexts
    \brief            User-friendly intermittent primitive representation
    \ingroup          pvr_primitives

    PVR Contexts are user-friendly, intermittent representations of primitive
    headers. Their members are filled in with the properties of the desired
    primitives, then they are compiled into headers which can be directly
    submitted to the PVR.

    \warning
    It is highly recommended that you cache the primitive headers that get
    compiled from these intermediate context types, so that you are not
    wasting time dynamically regenerating headers from contexts each frame.
*/

/** \defgroup pvr_ctx_attrib Attributes
    \brief                   PVR primitive context attributes
    \ingroup                 pvr_ctx
*/

/** \brief                          PowerVR primitive context shading modes
    \ingroup                        pvr_ctx_attrib

    Each polygon can define how it wants to be shaded, be it with flat or
    Gouraud shading using these constants in the appropriate place in its
    pvr_poly_cxt_t.
*/
typedef enum __packed pvr_shade {
    PVR_SHADE_FLAT    = 0, /**< \brief Use flat shading */
    PVR_SHADE_GOURAUD = 1  /**< \brief Use Gouraud shading */
} pvr_shade_t;

/** \defgroup pvr_ctx_depth     Depth
    \brief                      Depth attributes for PVR polygon contexts
    \ingroup                    pvr_ctx_attrib
*/

/** \brief                      PowerVR depth comparison modes
    \ingroup                    pvr_ctx_depth

    These set the depth function used for comparisons.
*/
typedef enum __packed pvr_depth {
    PVR_DEPTHCMP_NEVER    = 0,   /**< \brief Never pass */
    PVR_DEPTHCMP_LESS     = 1,   /**< \brief Less than */
    PVR_DEPTHCMP_EQUAL    = 2,   /**< \brief Equal to */
    PVR_DEPTHCMP_LEQUAL   = 3,   /**< \brief Less than or equal to */
    PVR_DEPTHCMP_GREATER  = 4,   /**< \brief Greater than */
    PVR_DEPTHCMP_NOTEQUAL = 5,   /**< \brief Not equal to */
    PVR_DEPTHCMP_GEQUAL   = 6,   /**< \brief Greater than or equal to */
    PVR_DEPTHCMP_ALWAYS   = 7    /**< \brief Always pass */
} pvr_depth_t;

/** \brief      PowerVR primitive context culling modes
    \ingroup    pvr_ctx_attrib

    These culling modes can be set by polygons to determine when they are
    culled. They work pretty much as you'd expect them to if you've ever used
    any 3D hardware before.
*/
typedef enum __packed pvr_cull {
    PVR_CULLING_NONE  = 0,   /**< \brief Disable culling */
    PVR_CULLING_SMALL = 1,   /**< \brief Cull if small */
    PVR_CULLING_CCW   = 2,   /**< \brief Cull if counterclockwise */
    PVR_CULLING_CW    = 3    /**< \brief Cull if clockwise */
} pvr_cull_t;

/** \defgroup pvr_ctx_texture Texture
    \brief                    Texture attributes for PVR polygon contexts
    \ingroup                  pvr_ctx_attrib
*/

/** \defgroup pvr_blend             Blending
    \brief                          Blending attributes for PVR primitive contexts
    \ingroup                        pvr_ctx_attrib
*/

/** \brief                          Blending modes for PowerVR primitive contexts
    \ingroup                        pvr_blend

    These are all the blending modes that can be done with regard to alpha
    blending on the PVR.
*/
typedef enum __packed pvr_blend {
    PVR_BLEND_ZERO         = 0,   /**< \brief None of this color */
    PVR_BLEND_ONE          = 1,   /**< \brief All of this color */
    PVR_BLEND_DESTCOLOR    = 2,   /**< \brief Destination color */
    PVR_BLEND_INVDESTCOLOR = 3,   /**< \brief Inverse of destination color */
    PVR_BLEND_SRCALPHA     = 4,   /**< \brief Blend with source alpha */
    PVR_BLEND_INVSRCALPHA  = 5,   /**< \brief Blend with inverse source alpha */
    PVR_BLEND_DESTALPHA    = 6,   /**< \brief Blend with destination alpha */
    PVR_BLEND_INVDESTALPHA = 7    /**< \brief Blend with inverse destination alpha */
} pvr_blend_t;

/** \brief                          PowerVR primitive context fog modes
    \ingroup                        pvr_ctx_attrib

    Each polygon can decide what fog type is used with regard to it using these
    constants in its pvr_poly_cxt_t.
*/
typedef enum __packed pvr_fog {
    PVR_FOG_TABLE   = 0,   /**< \brief Table fog */
    PVR_FOG_VERTEX  = 1,   /**< \brief Vertex fog */
    PVR_FOG_DISABLE = 2,   /**< \brief Disable fog */
    PVR_FOG_TABLE2  = 3    /**< \brief Table fog mode 2 */
} pvr_fog_t;

/** \brief                          PowerVR primitive context clipping modes
    \ingroup                        pvr_ctx_attrib

    These control how primitives are clipped against the user clipping area.
*/
typedef enum __packed pvr_clip {
    PVR_USERCLIP_DISABLE = 0,   /**< \brief Disable clipping */
    PVR_USERCLIP_INSIDE  = 2,   /**< \brief Enable clipping inside area */
    PVR_USERCLIP_OUTSIDE = 3    /**< \brief Enable clipping outside area */
} pvr_clip_t;

/** \defgroup pvr_ctx_color     Color
    \brief                      Color attributes for PowerVR primitive contexts
    \ingroup                    pvr_ctx_attrib
*/

/** \brief     Texture U/V flipping mode for the PVR
    \ingroup   pvr_ctx_texture

    These flags determine what happens when U/V coordinate values exceed 1.0.
    In any of the flipped cases, the specified coordinate value will flip around
    after 1.0, essentially mirroring the image. So, if you displayed an image
    with a U coordinate of 0.0 on the left hand side and 2.0 on the right hand
    side with U flipping turned on, you'd have an image that was displayed twice
    as if mirrored across the middle. This mirroring behavior happens at every
    unit boundary (so at 2.0 it returns to normal, at 3.0 it flips, etc).

    The default case is to disable mirroring. In addition, clamping of the U/V
    coordinates by PVR_UVCLAMP_U, PVR_UVCLAMP_V, or PVR_UVCLAMP_UV will disable
    the mirroring behavior.
*/
typedef enum __packed pvr_uv_flip {
    PVR_UVFLIP_NONE = 0,  /**< \brief No flipped coordinates */
    PVR_UVFLIP_V    = 1,  /**< \brief Flip V only */
    PVR_UVFLIP_U    = 2,  /**< \brief Flip U only */
    PVR_UVFLIP_UV   = 3   /**< \brief Flip U and V */
} pvr_uv_flip_t;

/** \brief   Enable or disable clamping of U/V on the PVR
    \ingroup pvr_ctx_texture

    These flags determine whether clamping will be applied to U/V coordinate
    values that exceed 1.0. If enabled, these modes will explicitly override the
    flip/mirroring modes (PVR_UVFLIP_U, PVR_UVFLIP_V, and PVR_UVFLIP_UV), and
    will instead ensure that the coordinate(s) in question never exceed 1.0.
*/
typedef enum __packed pvr_uv_clamp {
    PVR_UVCLAMP_NONE = 0,  /**< \brief Disable clamping */
    PVR_UVCLAMP_V    = 1,  /**< \brief Clamp V only */
    PVR_UVCLAMP_U    = 2,  /**< \brief Clamp U only */
    PVR_UVCLAMP_UV   = 3   /**< \brief Clamp U and V */
} pvr_uv_clamp_t;

/** \brief    PowerVR texture sampling modes
    \ingroup  pvr_ctx_texture
*/
typedef enum __packed pvr_filter {
    PVR_FILTER_NONE       = 0,   /**< \brief No filtering (point sample) */
    PVR_FILTER_NEAREST    = 0,   /**< \brief No filtering (point sample) */
    PVR_FILTER_BILINEAR   = 2,   /**< \brief Bilinear interpolation */
    PVR_FILTER_TRILINEAR1 = 4,   /**< \brief Trilinear interpolation pass 1 */
    PVR_FILTER_TRILINEAR2 = 6    /**< \brief Trilinear interpolation pass 2 */
} pvr_filter_t;

/** \brief    Mipmap bias modes for PowerVR primitive contexts
    \ingroup  pvr_ctx_texture
*/
typedef enum __packed pvr_mip_bias {
    PVR_MIPBIAS_NORMAL = 4,    /* txr_mipmap_bias */
    PVR_MIPBIAS_0_25   = 1,
    PVR_MIPBIAS_0_50   = 2,
    PVR_MIPBIAS_0_75   = 3,
    PVR_MIPBIAS_1_00   = PVR_MIPBIAS_NORMAL,
    PVR_MIPBIAS_1_25   = 5,
    PVR_MIPBIAS_1_50   = 6,
    PVR_MIPBIAS_1_75   = 7,
    PVR_MIPBIAS_2_00   = 8,
    PVR_MIPBIAS_2_25   = 9,
    PVR_MIPBIAS_2_50   = 10,
    PVR_MIPBIAS_2_75   = 11,
    PVR_MIPBIAS_3_00   = 12,
    PVR_MIPBIAS_3_25   = 13,
    PVR_MIPBIAS_3_50   = 14,
    PVR_MIPBIAS_3_75   = 15
} pvr_mip_bias_t;

/** \brief     PowerVR texture color calculation modes
    \ingroup   pvr_ctx_texture
*/
typedef enum __packed pvr_txr_env {
    PVR_TXRENV_REPLACE       = 0,   /**< \brief C = Ct, A = At */
    PVR_TXRENV_MODULATE      = 1,   /**< \brief C = Cs * Ct, A = At */
    PVR_TXRENV_DECAL         = 2,   /**< \brief C = (Cs * At) + (Cs * (1-At)), A = As */
    PVR_TXRENV_MODULATEALPHA = 3    /**< \brief C = Cs * Ct, A = As * At */
} pvr_txr_env_t;

/** \defgroup pvr_txr_fmts          Formats
    \brief                          PowerVR texture formats
    \ingroup                        pvr_txr_mgmt

    These are the texture formats that the PVR supports. Note that some of
    these, you can OR together with other values.

    @{
*/
#define PVR_TXRFMT_NONE          0                          /**< \brief No texture */
#define PVR_TXRFMT_MIPMAP       (1                   << 31) /**< Mip mapped (must be twiddled) */
#define PVR_TXRFMT_VQ_DISABLE   (0                   << 30) /**< Not VQ encoded */
#define PVR_TXRFMT_VQ_ENABLE    (1                   << 30) /**< VQ encoded */
#define PVR_TXRFMT_ARGB1555     (PVR_PIXFMT_ARGB1555 << 27) /**< 6-bit ARGB1555 */
#define PVR_TXRFMT_RGB565       (PVR_PIXFMT_RGB565   << 27) /**< 16-bit RGB565 */
#define PVR_TXRFMT_ARGB4444     (PVR_PIXFMT_ARGB4444 << 27) /**< 16-bit ARGB4444 */
#define PVR_TXRFMT_YUV422       (PVR_PIXFMT_YUV422   << 27) /**< YUV422 format */
#define PVR_TXRFMT_BUMP         (PVR_PIXFMT_BUMP     << 27) /**< Bumpmap format */
#define PVR_TXRFMT_PAL4BPP      (PVR_PIXFMT_PAL4BPP  << 27) /**< 4BPP paletted format */
#define PVR_TXRFMT_PAL8BPP      (PVR_PIXFMT_PAL8BPP  << 27) /**< 8BPP paletted format */
#define PVR_TXRFMT_TWIDDLED     (0                   << 26) /**< Texture is twiddled */
#define PVR_TXRFMT_NONTWIDDLED  (1                   << 26) /**< Texture is not twiddled */
#define PVR_TXRFMT_NOSTRIDE     (0                   << 21) /**< Texture is not strided */
#define PVR_TXRFMT_STRIDE       (1                   << 21) /**< Texture is strided */

/* OR one of these into your texture format if you need it. Note that
   these coincide with the twiddled/stride bits, so you can't have a
   non-twiddled/strided texture that's paletted! */

/** \brief   8BPP palette selector
    \param  x               The palette index */
#define PVR_TXRFMT_8BPP_PAL(x)  ((x) << 25)

/** \brief   4BPP palette selector
    \param  x               The palette index */
#define PVR_TXRFMT_4BPP_PAL(x)  ((x) << 21)
/** @} */

/** Packed bitfield describing a texture format. 

    \sa pvr_txr_fmts
*/
typedef uint32_t pvr_txr_fmt_t;

/** Different types of pixel formats supported by the PVR.

    \sa pvr_txr_fmts
 */
typedef enum pvr_pix_fmt {
    PVR_PIXFMT_ARGB1555 = 0,
    PVR_PIXFMT_RGB565   = 1,
    PVR_PIXFMT_ARGB4444 = 2,
    PVR_PIXFMT_YUV422   = 3,
    PVR_PIXFMT_BUMP     = 4,
    PVR_PIXFMT_PAL4BPP  = 5,
    PVR_PIXFMT_PAL8BPP  = 6
} pvr_pix_fmt_t;

/** \brief                          Color formats for PowerVR vertices
    \ingroup                        pvr_ctx_color

    These control how colors are represented in polygon data.
*/
typedef enum __packed pvr_color {
    PVR_CLRFMT_ARGBPACKED     = 0,  /**< \brief 32-bit integer ARGB */
    PVR_CLRFMT_4FLOATS        = 1,  /**< \brief 4 floating point values */
    PVR_CLRFMT_INTENSITY      = 2,  /**< \brief Intensity color */
    PVR_CLRFMT_INTENSITY_PREV = 3   /**< \brief Use last intensity */
} pvr_color_t;

/** \brief                          U/V data format for PVR textures
    \ingroup                        pvr_ctx_texture
*/
typedef enum __packed pvr_uv {
    PVR_UVFMT_32BIT = 0,  /**< \brief 32-bit floating point U/V */
    PVR_UVFMT_16BIT = 1   /**< \brief 16-bit floating point U/V */
} pvr_uv_t;

/** \defgroup pvr_ctx_modvol        Modifier Volumes
    \brief                          PowerVR modifier volume polygon context attributes
    \ingroup                        pvr_ctx_attrib
*/

/** \brief    Modifier volume modes for PowerVR primitive contexts
    \ingroup  pvr_ctx_modvol

    All triangles in a single modifier volume should be of the other poly type,
    except for the last one. That should be either of the other two types,
    depending on whether you want an inclusion or exclusion volume.
*/
typedef enum __packed pvr_mod {
    PVR_MODIFIER_OTHER_POLY        = 0,  /**< \brief Not the last polygon in the volume */
    PVR_MODIFIER_INCLUDE_LAST_POLY = 1,  /**< \brief Last polygon, inclusion volume */
    PVR_MODIFIER_EXCLUDE_LAST_POLY = 2   /**< \brief Last polygon, exclusion volume */
} pvr_mod_t;

/** \brief   PVR polygon context.
    \ingroup pvr_ctx

    Human-readable intermediate representation of a PVR polygon header. This
    structure should be filled in depending on the desired polygon properties
    then compiled into a pvr_poly_hdr_t, which can be directly submitted.

    \sa pvr_sprite_ctx_t, pvr_poly_hdr_t
*/
typedef struct pvr_poly_cxt {
    pvr_list_t        list_type;     /**< List type to submit */
    struct {
        bool          alpha;         /**< Enable alpha outside modifier */
        bool          shading;       /**< Enable gourad shading */
        pvr_fog_t     fog_type;      /**< Fog type outside modifier */
        pvr_cull_t    culling;       /**< Culling mode */
        bool          color_clamp;   /**< Enable color clamping outside of modifer */
        pvr_clip_t    clip_mode;     /**< Clipping mode */
        pvr_mod_t     modifier_mode; /**< Modifier mode */
        bool          specular;      /**< Enable offset color outside of modifier */
        bool          alpha2;        /**< Enable alpha inside modifier */
        pvr_fog_t     fog_type2;     /**< Fog type inside modifier */
        bool          color_clamp2;  /**< Enable color clamping inside modifier */
    } gen;                           /**< General parameters */
    struct {
        pvr_blend_t    src;          /**< Source blending mode outside modifier */
        pvr_blend_t    dst;          /**< Dest blending mode outside modifier */
        bool           src_enable;   /**< Source blending enable outside modifier */
        bool           dst_enable;   /**< Dest blending enable outside modifier */
        pvr_blend_t    src2;         /**< Source blending mode inside modifier */
        pvr_blend_t    dst2;         /**< Dest blending mode inside modifier */
        bool           src_enable2;  /**< Src blending enable inside modifier */
        bool           dst_enable2;  /**< Dest blending enable inside modifier */
    } blend;                         /**< Blending parameters */
    struct {
        pvr_color_t    color;        /**< Color format in vertex */
        pvr_uv_t       uv;           /**< U/V data format in vertex */
        bool           modifier;     /**< Enable or disable modifier effect */
    } fmt;                           /**< Format control */
    struct {
        pvr_depth_t    comparison;   /**< Depth comparison mode */
        bool           write;        /**< Enable depth writes */
    } depth;                         /**< Depth comparison/write modes */
    struct {
        bool           enable;       /**< Enable/disable texturing */
        pvr_filter_t   filter;       /**< Filtering mode */
        bool           mipmap;       /**< Enable/disable mipmaps */
        pvr_mip_bias_t mipmap_bias;  /**< Mipmap bias */
        pvr_uv_flip_t  uv_flip;      /**< Enable/disable U/V flipping */
        pvr_uv_clamp_t uv_clamp;     /**< Enable/disable U/V clamping */
        bool           alpha;        /**< Enable/disable texture alpha */
        pvr_txr_env_t  env;          /**< Texture color contribution */
        size_t         width;        /**< Texture width (pow-of-2 required) */
        size_t         height;       /**< Texture height (pow-of-2 required) */
        pvr_txr_fmt_t  format;       /**< Texture format */
        pvr_ptr_t      base;         /**< Texture pointer */
    } txr,                           /**< Texturing params outside modifier */
      txr2;                          /**< Texturing params inside modifier */
} pvr_poly_cxt_t;

/** \brief   PVR sprite context.
    \ingroup pvr_ctx

    You should use this more human readable format for specifying your sprite
    contexts, and then compile them into sprite headers when you are ready to
    start using them.
*/
typedef struct pvr_sprite_ctx {
    pvr_list_t         list_type;   /**< Primitive list */
    struct {
        bool           alpha;       /**< Enable or disable alpha */
        pvr_fog_t      fog_type;    /**< Fog type */
        pvr_cull_t     culling;     /**< Culling mode */
        bool           color_clamp; /**< Color clamp enable/disable */
        pvr_clip_t     clip_mode;   /**< Clipping mode */
        bool           specular;    /**< Offset color enable/disable */
    } gen;                          /**< General parameters */
    struct {
        pvr_blend_t    src;         /**< Source blending mode */
        pvr_blend_t    dst;         /**< Dest blending mode */
        bool           src_enable;  /**< Source blending enable */
        bool           dst_enable;  /**< Dest blending enable */
    } blend;                        /**< Blending parameters */
    struct {
        pvr_depth_t    comparison;  /**< Depth comparison mode  */
        bool           write;       /**< Enable or disable depth writes */
    } depth;                        /**< Depth comparison/write modes */
    struct {
        bool           enable;      /**< Enable/disable texturing */
        pvr_filter_t   filter;      /**< Filtering mode */
        bool           mipmap;      /**< Enable/disable mipmaps */
        pvr_mip_bias_t mipmap_bias; /**< Mipmap bias */
        pvr_uv_flip_t  uv_flip;     /**< Enable/disable U/V flipping */
        pvr_uv_clamp_t uv_clamp;    /**< Enable/disable U/V clamping */
        bool           alpha;       /**< Enable/disable texture alpha */
        pvr_txr_env_t  env;         /**< Texture color contribution */
        size_t         width;       /**< Texture width (pow-of-2 required) */
        size_t         height;      /**< Texture height (pow-of-2 required) */
        pvr_txr_fmt_t  format;      /**< Texture format */
        pvr_ptr_t      base;        /**< Texture pointer */
    } txr;                          /**< Texturing params */
} pvr_sprite_cxt_t;

/* Primitive handling ************************************************/

/** \defgroup pvr_primitives_compilation Compilation
    \brief                               API for compiling primitive contexts
                                         into headers
    \ingroup pvr_ctx
*/

/** \brief   Compile a polygon context into a polygon header.
    \ingroup pvr_primitives_compilation

    This function compiles a pvr_poly_cxt_t into the form needed by the hardware
    for rendering. This is for use with normal polygon headers.

    \param  dst             Where to store the compiled header.
    \param  src             The context to compile.
*/
void pvr_poly_compile(pvr_poly_hdr_t *dst, const pvr_poly_cxt_t *src);

/** \defgroup pvr_ctx_init     Initialization
    \brief                     Functions for initializing PVR polygon contexts
    \ingroup                   pvr_ctx
*/

/** \brief   Fill in a polygon context for non-textured polygons.
    \ingroup pvr_ctx_init

    This function fills in a pvr_poly_cxt_t with default parameters appropriate
    for rendering a non-textured polygon in the given list.

    \param  dst             Where to store the polygon context.
    \param  list            The primitive list to be used.
*/
void pvr_poly_cxt_col(pvr_poly_cxt_t *dst, pvr_list_t list);

/** \brief   Fill in a polygon context for a textured polygon.
    \ingroup pvr_ctx_init

    This function fills in a pvr_poly_cxt_t with default parameters appropriate
    for rendering a textured polygon in the given list.

    \param  dst             Where to store the polygon context.
    \param  list            The primitive list to be used.
    \param  textureformat   The format of the texture used.
    \param  tw              The width of the texture, in pixels.
    \param  th              The height of the texture, in pixels.
    \param  textureaddr     A pointer to the texture.
    \param  filtering       The type of filtering to use.

    \see    pvr_txr_fmts
    \see    pvr_filter_modes
*/
void pvr_poly_cxt_txr(pvr_poly_cxt_t *dst, pvr_list_t list,
                      pvr_txr_fmt_t textureformat, size_t tw, size_t th, pvr_ptr_t textureaddr,
                      pvr_filter_t filtering);

/** \brief   Compile a sprite context into a sprite header.
    \ingroup pvr_primitives_compilation

    This function compiles a pvr_sprite_cxt_t into the form needed by the
    hardware for rendering. This is for use with sprite headers.

    \param  dst             Where to store the compiled header.
    \param  src             The context to compile.
*/
void pvr_sprite_compile(pvr_sprite_hdr_t *dst,
                        const pvr_sprite_cxt_t *src);

/** \brief   Fill in a sprite context for non-textured sprites.
    \ingroup pvr_ctx_init

    This function fills in a pvr_sprite_cxt_t with default parameters
    appropriate for rendering a non-textured sprite in the given list.

    \param  dst             Where to store the sprite context.
    \param  list            The primitive list to be used.
*/
void pvr_sprite_cxt_col(pvr_sprite_cxt_t *dst, pvr_list_t list);

/** \brief   Fill in a sprite context for a textured sprite.
    \ingroup pvr_ctx_init

    This function fills in a pvr_sprite_cxt_t with default parameters
    appropriate for rendering a textured sprite in the given list.

    \param  dst             Where to store the sprite context.
    \param  list            The primitive list to be used.
    \param  textureformat   The format of the texture used.
    \param  tw              The width of the texture, in pixels.
    \param  th              The height of the texture, in pixels.
    \param  textureaddr     A pointer to the texture.
    \param  filtering       The type of filtering to use.

    \see    pvr_txr_fmts
    \see    pvr_filter_modes
*/
void pvr_sprite_cxt_txr(pvr_sprite_cxt_t *dst, pvr_list_t list,
                        pvr_txr_fmt_t textureformat, size_t tw, size_t th, pvr_ptr_t textureaddr,
                        pvr_filter_t filtering);

/** \brief   Create a modifier volume header.
    \ingroup pvr_primitives_compilation

    This function fills in a modifier volume header with the parameters
    specified. Note that unlike for polygons and sprites, there is no context
    step for modifiers.

    \param  dst             Where to store the modifier header.
    \param  list            The primitive list to be used.
    \param  mode            The mode for this modifier.
    \param  cull            The culling mode to use.

    \see    pvr_mod_modes
    \see    pvr_cull_modes
*/
void pvr_mod_compile(pvr_mod_hdr_t *dst, pvr_list_t list, pvr_mod_t mode,
                     pvr_cull_t cull);

/** \brief   Compile a polygon context into a polygon header that is affected by
             modifier volumes.
    \ingroup pvr_primitives_compilation

    This function works pretty similarly to pvr_poly_compile(), but compiles
    into the header type that is affected by a modifier volume. The context
    should have been created with either pvr_poly_cxt_col_mod() or
    pvr_poly_cxt_txr_mod().

    \param  dst             Where to store the compiled header.
    \param  src             The context to compile.
*/
void pvr_poly_mod_compile(pvr_poly_mod_hdr_t *dst, const pvr_poly_cxt_t *src);

/** \brief   Fill in a polygon context for non-textured polygons affected by a
             modifier volume.
    \ingroup pvr_ctx_init

    This function fills in a pvr_poly_cxt_t with default parameters appropriate
    for rendering a non-textured polygon in the given list that will be affected
    by modifier volumes.

    \param  dst             Where to store the polygon context.
    \param  list            The primitive list to be used.
*/
void pvr_poly_cxt_col_mod(pvr_poly_cxt_t *dst, pvr_list_t list);

/** \brief   Fill in a polygon context for a textured polygon affected by
             modifier volumes.
    \ingroup pvr_ctx_init

    This function fills in a pvr_poly_cxt_t with default parameters appropriate
    for rendering a textured polygon in the given list and being affected by
    modifier volumes.

    \param  dst             Where to store the polygon context.
    \param  list            The primitive list to be used.
    \param  textureformat   The format of the texture used (outside).
    \param  tw              The width of the texture, in pixels (outside).
    \param  th              The height of the texture, in pixels (outside).
    \param  textureaddr     A pointer to the texture (outside).
    \param  filtering       The type of filtering to use (outside).
    \param  textureformat2  The format of the texture used (inside).
    \param  tw2             The width of the texture, in pixels (inside).
    \param  th2             The height of the texture, in pixels (inside).
    \param  textureaddr2    A pointer to the texture (inside).
    \param  filtering2      The type of filtering to use (inside).

    \see    pvr_txr_fmts
    \see    pvr_filter_modes
*/
void pvr_poly_cxt_txr_mod(pvr_poly_cxt_t *dst, pvr_list_t list,
                          pvr_txr_fmt_t textureformat, size_t tw, size_t th,
                          pvr_ptr_t textureaddr, pvr_filter_t filtering,
                          pvr_txr_fmt_t textureformat2, size_t tw2, size_t th2,
                          pvr_ptr_t textureaddr2, pvr_filter_t filtering2);

__END_DECLS

#endif
