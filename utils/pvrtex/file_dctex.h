#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifndef __always_inline
/** \brief  Ask the compiler to \a always inline a given function. */
#define __always_inline inline __attribute__((__always_inline__))
#endif

//These match up with the PVR hardware texture format bits
typedef enum fdtPixelFormat {
	FDT_FMT_ARGB1555,
	FDT_FMT_RGB565,
	FDT_FMT_ARGB4444,
	FDT_FMT_YUV,
	FDT_FMT_NORMAL,
	FDT_FMT_PALETTE_4BPP,
	FDT_FMT_PALETTE_8BPP,
} fdtPixelFormat;

#define FDT_PVR_SIZE_MASK	0x0000003F
#define FDT_PVR_MODE_MASK	0xFC000000
#define FDT_PVR_MODE_PAL_MASK	0xFFE00000
#define FDT_CODEBOOK_MAX_SIZE_BYTES	2048

#define FDT_MIPMAP_SHIFT	31
#define FDT_MIPMAP_MASK	1
#define FDT_VQ_SHIFT		30
#define FDT_VQ_MASK		1
#define FDT_PIXEL_FORMAT_MASK	0x7
#define FDT_PIXEL_FORMAT_SHIFT	27
#define FDT_NOT_TWIDDLED_SHIFT	26
#define FDT_NOT_TWIDDLED_MASK	1
#define FDT_STRIDE_SHIFT	25
#define FDT_STRIDE_MASK	1
#define FDT_PIXEL_FORMAT_MASK	0x7
#define FDT_PARTIAL_SHIFT	11
#define FDT_PARTIAL_MASK	1
#define FDT_STRIDE_VAL_SHIFT	6
#define FDT_STRIDE_VAL_MASK	0x1F
#define FDT_WIDTH_SHIFT	3
#define FDT_WIDTH_MASK		0x7
#define FDT_HEIGHT_SHIFT	0
#define FDT_HEIGHT_MASK		0x7

typedef struct {
	/*
		All data is little endian (same as the Dreamcast's SH-4)
	
		Header is 32 bytes large
	*/

	/*
		Four character code to identify file format.
		Always equal to "DcTx"
	*/
	char fourcc[4];

	/*
		Size of file, including header. This is different to IFF, which does not include the size of the fourcc and size fields.
	
		Size is always rounded up to 32 bytes
	*/
	uint32_t chunk_size;

	/*
		File format version of this texture.
	
		Currently, the only version is 0
	*/
	uint8_t version;

	/*
		Size of the header in 32-byte units, minus one. This is how far from the start of the header the texture data starts.
		The header can be 32 bytes to 8KB large.
	
		A header_size of 0 means the texture data starts 32 bytes after the start of the header, a size of 3 means 128 bytes...
	
		This allows for backwards compatible changes to the size of the header, or adding additional user data.
	*/
	uint8_t header_size;

	/*
		Values range from 0-255, mapping to 1-256. To get the real number of codebook entries or colors,
		add one to this value. (e.g. if colors_used == 15, then there are 16 different colors, if,
		colors_used == 2, then 2 colors are used)
	
		The value of codebook_size is undefined if not compressed, and should not be relied upon.
		colors_used is similarly undefined if pixel format is not 8BPP or 4BPP.
	
		The functions fDtGetColorsUsed and fDtGetColorsUsed can be used to preform the checks and offseting.
		They return 0 if not palletized or compressed
	*/
	uint8_t codebook_size, colors_used;

	/*
		Height and width of the texture in pixels.
	
		This might not match up with the sizes in pvr_size.
	
		Stride textures will have the true width here, but the size in pvr_size
		will be rounded up to the next power of two.
	
		It's possible to have non-twiddled textures with a height that is not a power of two,
		the size in pvr_size will be rounded up to the next power of two. The PVR does not
		have a texture format that supports this, but if you don't display any texels from
		outside the real area, it will still work.
	*/
	uint16_t width_pixels, height_pixels;

	/*
		The bottom 6 bits match up with the third long in the PVR's triangle/quad command,
		and the top 10 bits match up with the fourth long.
	
		Bit 31:
			Mipmapped (0 = no mipmaps, 1 = has mipmaps)
		
		Bit 30:
			VQ compression (0 = not compressed, 1 = is compressed)
		
		Bits 29-27:
			Pixel format
				0: ARGB1555
				1: RGB565
				2: ARGB4444
				3: YUV422
				4: Spherical normal
				5: 4-bit palette
				6: 8-bit palette
	
		For pixel formats other than palettized:
			Bit 26:
				Not twiddled (0 = twiddled, 1 = not twiddled)
		
			Bit 25:
				Strided (0 = width is power of two, 1 = width is not power of two)

		For palettized pixel formats:
			Bit 26-21:
				Palette hint (This is currently always 0, but could be used in the future)
		
			Palettized textures are always assumed to be twiddled and never strided by the hardware
	
		Bit 11:
			Partial texture (0 = not partial, 1 = partial)
		
			A partial texture is a texture that does not have all data to fill the PVRs view of the texture.
			This saves video RAM, but can result in the PVR reading garbage if you try to display texels from outside
			the defined range. The undefined data in a partial texture is always at the end of the texture, never the 
			middle or start. (Small codebook VQ textures would techincally qualify as a type of partical texture with
			some of the data at the start of a texture missing, they are not considered to be partial for the purposes
			of this bit.)
		
			For nontwiddled data, a partial texture results in the bottom rows of the texture being undefined. For
			twiddled textures, this results in the topmost mip level having data in the right half being undefined.
		
			For example, a 640x480 texture can at best be seen by the PVR as a 640x512 texture. Trying to display
			data from below the 480th row will result in the PVR reading undefined data.
		
			For a twiddled, mipmapped texture, a partial texture might be missing the bottom right corner or
			or entire right half the highest mip level, or other shapes. This type is not currently supported by the
			converter.
		
		Bits 10-6:
			Stride value. This is the width of the texture divided by 32.
			Only valid on texture will stride bit set.
			Place this value in the bottom 5 bits of the PVR register PVR_TEXTURE_MODULO (at address 0xA05F80E4).
			Only one stride value can be used per frame.
	
		Bits 5-3:
			Texture width
			Value    Pixels
			  0         8
			  1        16
			  2        32
			  3        64
			  4       128
			  5       256
			  6       512
			  7      1024
		
			For stride textures, this will be set to the next size larger than the stride value.
			For example, a 640 pixel wide texture will have a width of 7 (1024).
		
		Bits 2-0:
			Texture height
			Value    Pixels
			  0         8
			  1        16
			  2        32
			  3        64
			  4       128
			  5       256
			  6       512
			  7      1024
		
			For textures with a height that is not a power of two, the value here will be rounded up.
			For example, a 480 pixel high texture will have a height of 6 (512).
	
	*/
	uint32_t pvr_type;

	/*
		Pad header out to 32 bytes
	
		DMA requires 32 byte alignment.
	
		Padding might be used in future versions as an identifer or hash of the texture,
		to help track identical textures, or store other user data like material properties.
	*/
	uint32_t pad1;

	uint32_t pad2;

	uint32_t pad3;

	//Texture data follows...
} fDtHeader;

/*
	Returns true if the fourcc matches
*/
static inline bool fDtFourccMatches(const fDtHeader *tex) {
	const int *fourcc = (const int *)&tex->fourcc;
	return *fourcc == 0x78546344;	//'DxTc'

	/* return tex->fourcc[0] == 'D' && 
		tex->fourcc[1] == 'c' && 
		tex->fourcc[2] == 'T' && 
		tex->fourcc[3] == 'x'; */
}

/*
	Returns version of texture
*/
static inline int fDtGetVersion(const fDtHeader *tex) {
	return tex->version;
}

/*
	Returns size of file, including header and texture data. Will always be a multiple of 32 (assuming valid texture).
*/
static inline size_t fDtGetTotalSize(const fDtHeader *tex) {
	return tex->chunk_size;
}

/*
	Returns the size of the header in bytes
*/
static inline size_t fDtGetHeaderSize(const fDtHeader *tex) {
	return (tex->header_size+1) * 32;
}

/*
	Returns size of texture data. Does not include header. Will always be a multiple of 32 (assuming valid texture).
*/
static inline size_t fDtGetTextureSize(const fDtHeader *tex) {
	return fDtGetTotalSize(tex) - fDtGetHeaderSize(tex);
}

/*
	Returns pointer to end of texture (byte after final byte of texture)
*/
static inline void * fDtGetNextChunk(const fDtHeader *tex) {
	return (void*)tex + fDtGetTotalSize(tex);
}


/*
	Returns true width of texture in pixels.

	The value that needs to be passed to the PVR might be different than this. Use
	fDtGetPvrWidthBits to get the value that should be given to the PVR to correctly
	use the texture.

*/
static inline unsigned fDtGetWidth(const fDtHeader *tex) {
	return tex->width_pixels;
}
/*
	Returns true height of texture in pixels.

	The value that needs to be passed to the PVR might be different than this. Use
	fDtGetPvrHeightBits to get the value that should be given to the PVR to correctly
	use the texture.
*/
static inline unsigned fDtGetHeight(const fDtHeader *tex) {
	return tex->height_pixels;
}

/*
	Returns the PVR pixel format of the texture. You can pass this directly to the PVR as the texture format.
*/
static inline unsigned fDtGetPixelFormat(const fDtHeader *tex) {
	return (tex->pvr_type >> FDT_PIXEL_FORMAT_SHIFT) & FDT_PIXEL_FORMAT_MASK;
}

/*
	Returns true if the texture is palettized (i.e. texture is 4BPP or 8BPP).
*/
static inline int fDtIsPalettized(const fDtHeader *tex) {
	unsigned fmt = fDtGetPixelFormat(tex);
	return fmt == FDT_FMT_PALETTE_8BPP || fmt == FDT_FMT_PALETTE_4BPP;
}

/*
	Returns the stride value of the texture.

	Result is undefined if texture is not strided.
*/
static inline unsigned int fDtGetStride(const fDtHeader *tex) {
	return ((tex->pvr_type >> FDT_STRIDE_VAL_SHIFT)  & FDT_STRIDE_VAL_MASK);
}

/*
	Returns true if the texture is a partial texture (see format description for expination of partial textures),
	and returns false if the texture is complete.
*/
static inline int fDtIsPartial(const fDtHeader *tex) {
	return (tex->pvr_type  & (1<<FDT_PARTIAL_SHIFT)) != 0;
}

/*
	Returns true if the texture is a stride texture (not power of two width), or false if the
	texture has a power of two width.
*/
static inline int fDtIsStrided(const fDtHeader *tex) {
	return ((tex->pvr_type  & (1<<FDT_STRIDE_SHIFT)) != 0) && !fDtIsPalettized(tex);
}

/*
	Returns true if the texture is twiddled, or false if the texture is not.
*/
static inline int fDtIsTwiddled(const fDtHeader *tex) {
	return fDtIsPalettized(tex) || ((tex->pvr_type  & (1<<26)) == 0);
}

/*
	Returns true if the texture is compressed, or false if it is not.
*/
static inline int fDtIsCompressed(const fDtHeader *tex) {
	return (tex->pvr_type  & (1<<FDT_VQ_SHIFT)) != 0;
}

/*
	Returns true if the texture is mipmapped, or false if it is not.
*/
static inline int fDtIsMipmapped(const fDtHeader *tex) {
	return ((int32_t)tex->pvr_type) < 0;
}

/*
	Returns the width value to written to the TA command to use for the texture.
*/
static inline unsigned fDtGetPvrWidthBits(const fDtHeader *tex) {
	return (tex->pvr_type >> FDT_WIDTH_SHIFT) & FDT_WIDTH_MASK;
}
/*
	Returns the height value to written to the TA command to use for the texture.
*/
static inline unsigned fDtGetPvrHeightBits(const fDtHeader *tex) {
	return (tex->pvr_type >> FDT_HEIGHT_SHIFT) & FDT_HEIGHT_MASK;
}

/*
	Returns how wide/high the PVR thinks the texture is in pixels.

	This is needed to figure out the correct U value for strided textures.
	You can use fDtGetUWidth or fDtGetVHeight to find the correct value for
	the edge of the texture.
*/
static inline unsigned fDtGetPvrWidth(const fDtHeader *tex) {
	return 1 << (fDtGetPvrWidthBits(tex) + 3);
}
static inline unsigned fDtGetPvrHeight(const fDtHeader *tex) {
	return 1 << (fDtGetPvrHeightBits(tex) + 3);
}

/*
	Returns U value for right edge of texture.

	This can be used to map the entire valid area of the texture to a polygon, with
	the top left UV coord being (0, 0), and the bottom right UV coord
	being (fDtGetUWidth(tex), fDtGetVHeight(tex))
*/
static inline float fDtGetUWidth(const fDtHeader *tex) {
	return (float)fDtGetWidth(tex) / fDtGetPvrWidth(tex);
}

/*
	Returns V value for bottom edge of texture

	See fDtGetUWidth description
*/
static inline float fDtGetVHeight(const fDtHeader *tex) {
	return (float)fDtGetHeight(tex) / fDtGetPvrHeight(tex);
}

/*
	Returns number of colors used by palettized texture.

	Returns 0 if texture does not use a palette.
*/
static inline unsigned fDtGetColorsUsed(const fDtHeader *tex) {
	return fDtIsPalettized(tex) ? tex->colors_used+1 : 0;
}

/*
	Returns size of codebook for compressed texture in bytes.

	Returns 0 if texture is not compressed.
*/
static inline unsigned fDtGetCodebookSizeBytes(const fDtHeader *tex) {
	return fDtIsCompressed(tex) ? (tex->codebook_size+1) * 8 : 0;
}

/*
	Returns pointer to pixel data
*/
static inline void * fDtGetPvrTexData(const fDtHeader *tex) {
	return (void*)tex + fDtGetHeaderSize(tex);
}

/*
	Preforms sanity checking on header to guess if it is valid.

	Checks fourcc, version, size, and texture format.

	Returns true if checks passes, or false if any failed.

	This is too large to be made static inline like the other functions in here,
	but seperating out one function to a .c file would be a pain, so it's
	just static. The compiler can get rid of it if it's not used.
	The unused attribute is to disable any warnings if this is not used.
*/
static bool __attribute__((unused)) fDtValidateHeader(const fDtHeader *tex) {
	bool valid = true;

	//Check fourcc matches
	valid &= fDtFourccMatches(tex);

	//Currently, only version is 0. There will probably not be more than 50 versions,
	//so anything more than that is suspicious
	valid &= fDtGetVersion(tex) < 50;

	//Size should be multiple of 32
	valid &= (fDtGetTotalSize(tex) % 32) == 0;

	//Check texture dimensions
	valid &= fDtGetWidth(tex) >= 8;
	valid &= fDtGetWidth(tex) <= 1024;
	valid &= fDtGetPvrWidth(tex) >= fDtGetWidth(tex);
	valid &= fDtGetHeight(tex) > 0;
	valid &= fDtGetHeight(tex) <= 1024;
	valid &= fDtGetPvrHeight(tex) >= fDtGetHeight(tex);

	//Check size is expected value
	unsigned size = fDtGetWidth(tex)*fDtGetHeight(tex)*2;

	//Calculate size of texture
	if (fDtIsMipmapped(tex))
		size = size * 4/3 + 6;
	if (fDtGetPixelFormat(tex) == FDT_FMT_PALETTE_8BPP)
		size /= 2;
	else if (fDtGetPixelFormat(tex) == FDT_FMT_PALETTE_4BPP)
		size /= 4;
	if (fDtIsCompressed(tex))
		size = (size+7)/8 + fDtGetCodebookSizeBytes(tex);
	//Round up to 32 bytes
	size = (size + 31) & ~0x1f;
	//Add header size
	size += fDtGetHeaderSize(tex);
	valid &= fDtGetTotalSize(tex) == size;

	//Check valid pixel format
	valid &= fDtGetPixelFormat(tex) <= FDT_FMT_PALETTE_8BPP;

	//If strided texture has height is not equal to PVR height, must be partial
	if (fDtIsStrided(tex) && (fDtGetHeight(tex) != fDtGetPvrHeight(tex)))
		valid &= fDtIsPartial(tex);

	return valid;
}

#ifdef _arch_dreamcast
/*
	Small codebook compressed textures require an adjustment to the texture pointer for the PVR to
	render it correctly. This function will preform the adjustment.

	For compressed textures, returns adjusted pointer to video RAM
	For uncompressed textures, returns pointer unchanged.

	This does not modify the texture or it's allocation in any way, so when freeing the texture, be
	sure to use the original, unadjusted pvr_ptr_t pointer.
*/
static inline pvr_ptr_t fDtAdjustPVRPointer(const fDtHeader * texheader, pvr_ptr_t pvr) {
	if (fDtIsCompressed(texheader)) {
		return pvr - FDT_CODEBOOK_MAX_SIZE_BYTES + fDtGetCodebookSizeBytes(texheader);
	} else {
		return pvr;
	}
}

/*
	Set PVR hardware register for stride to value required for this texture

	You want to avoid calling this while rendering, but KOS doesn't really provide a way to do that...
	A work around would be to only set the stride value on a frame wher  no strided texture is being rendered.
*/
static inline void fDtSetPvrStride(const fDtHeader *tex) {
	if (fDtIsStrided(tex))
		PVR_SET(PVR_TEXTURE_MODULO, fDtGetStride(tex));
}

/*
	These set the texture for a KOS compiled polygon header struct.

	The format of the texture is taken from tex, and the actual address of the texture is
	set by video_ram_addr.
*/
static inline void fDtSetTAParameters(pvr_poly_hdr_t *dst, const fDtHeader *tex, pvr_ptr_t video_ram_addr) {
	dst->mode2 = (dst->mode2 & ~FDT_PVR_SIZE_MASK) | (tex->pvr_type & FDT_PVR_SIZE_MASK);
	dst->mode3 = (tex->pvr_type & FDT_PVR_MODE_PAL_MASK);
	dst->mode3 |= 0x1ffffff & ((unsigned)fDtAdjustPVRPointer(tex, video_ram_addr) >> 3);
}
static inline void fDtSetTAParametersIC(pvr_poly_ic_hdr_t *dst, const fDtHeader *tex, pvr_ptr_t video_ram_addr) {
	dst->mode2 = (dst->mode2 & ~FDT_PVR_SIZE_MASK) | (tex->pvr_type & FDT_PVR_SIZE_MASK);
	dst->mode3 = (tex->pvr_type & FDT_PVR_MODE_PAL_MASK);
	dst->mode3 |= 0x1ffffff & ((unsigned)fDtAdjustPVRPointer(tex, video_ram_addr) >> 3);
}
static inline void fDtSetTAParametersSprite(pvr_sprite_hdr_t *dst, const fDtHeader *tex, pvr_ptr_t video_ram_addr) {
	dst->mode2 = (dst->mode2 & ~FDT_PVR_SIZE_MASK) | (tex->pvr_type & FDT_PVR_SIZE_MASK);
	dst->mode3 = (tex->pvr_type & FDT_PVR_MODE_PAL_MASK);
	dst->mode3 |= 0x1ffffff & ((unsigned)fDtAdjustPVRPointer(tex, video_ram_addr) >> 3);
}
static inline void fDtSetTAParametersMod(pvr_poly_mod_hdr_t *dst, const fDtHeader *tex, pvr_ptr_t video_ram_addr, int param) {
	if (param == 0) {
		dst->mode2_0 = (dst->mode2_0 & ~FDT_PVR_SIZE_MASK) | (tex->pvr_type & FDT_PVR_SIZE_MASK);
		dst->mode3_0 = (tex->pvr_type & FDT_PVR_MODE_PAL_MASK);
		dst->mode3_0 |= 0x1ffffff & ((unsigned)fDtAdjustPVRPointer(tex, video_ram_addr) >> 3);
	} else {
		dst->mode2_1 = (dst->mode2_1 & ~FDT_PVR_SIZE_MASK) | (tex->pvr_type & FDT_PVR_SIZE_MASK);
		dst->mode3_1 = (tex->pvr_type & FDT_PVR_MODE_PAL_MASK);
		dst->mode3_1 |= 0x1ffffff & ((unsigned)fDtAdjustPVRPointer(tex, video_ram_addr));
	}
}

#ifdef PVR_CXT_GUARD
/*
	This is for my pvr_context library. pvr_cxt.h must be included beforehand.
*/
static inline void fDtSetPvrContextMod(pvr_context_submodes *dst, const fDtHeader *tex, pvr_ptr_t video_ram_addr) {
	dst->mode2 = (dst->mode2 & ~FDT_PVR_SIZE_MASK) | (tex->pvr_type & FDT_PVR_SIZE_MASK);
	dst->tex = (tex->pvr_type & FDT_PVR_MODE_PAL_MASK);
	pc_set_texture_address_mod(dst, fDtAdjustPVRPointer(tex, video_ram_addr));
}
static inline void fDtSetPvrContext(pvr_context *dst, const fDtHeader *tex, pvr_ptr_t video_ram_addr) {
	pc_set_textured(dst, 1);
	fDtSetPvrContextMod(pc_no_mod(dst), tex, video_ram_addr);
}
#endif

#endif
