#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "pvr_texture_decoder.h"
#include "pvr_texture_encoder.h"

/*
	Returns the pixel format for a given mipmap level
	YUV can change depending on level and if the texture is twiddled, this function
	returns the correct format.
	
	Passing 1 for miplevel will can be used to get the format for any level that
	isn't the 1x1 level
*/
static unsigned ptdGetConvertFormat(PvrTexDecoder *ptd, int miplevel) {
	assert(ptd);
	
	unsigned format = ptd->pixel_format;
	if (format == PT_YUV) {
		if (miplevel == 0 && ptd->mips)
			return PT_RGB565;
		if (!ptd->stride)
			return PT_YUV_TWID;
	}
	return format;
}

void ptdInit(PvrTexDecoder *ptd) {
	memset(ptd, 0, sizeof(*ptd));
	
	
	for(unsigned i = 0; i < PVR_8B_PALETTE_SIZE; i++) {
		float c = i / 255.0f;
		ptd->palette[i] = pxlSetABGR8888(255.0f, c, c, c);
	}
}

void ptdFree(PvrTexDecoder *ptd) {
	for(unsigned i = 0; i < PVR_MAX_MIPMAPS; i++) 
		SAFE_FREE(&ptd->result_mips[i]);
}

void ptdSetSize(PvrTexDecoder *ptd, unsigned w, unsigned h, bool mips) {
	ptd->w = w;
	ptd->h = h;
	ptd->mips = mips;
	ptd->mip_cnt = mips ? MipLevels(ptd->w) : 1;
}

void ptdSetPixelFormat(PvrTexDecoder *ptd, ptPixelFormat pixel_format) {
	ptd->pixel_format = pixel_format;
	
	if (pixel_format == PT_PALETTE_8B) {
		for(unsigned i = 0; i < PVR_8B_PALETTE_SIZE; i++) {
			float c = i / (float)(PVR_8B_PALETTE_SIZE-1);
			ptd->palette[i] = pxlSetABGR8888(255.0f, c, c, c);
		}
	} else if (pixel_format == PT_PALETTE_4B) {
		for(unsigned i = 0; i < PVR_4B_PALETTE_SIZE; i++) {
			float c = i / (float)(PVR_4B_PALETTE_SIZE-1);
			ptd->palette[i] = pxlSetABGR8888(255.0f, c, c, c);
		}
	}
}


void ptdSetStride(PvrTexDecoder *ptd, bool stride) {
	ptd->stride = stride;
}

void ptdSetUncompressedSource(PvrTexDecoder *ptd, const void *pixels) {
	ptd->tex_data = pixels;
}

void ptdSetCompressedSource(PvrTexDecoder *ptd, const void *indices, const void *cb, size_t cb_size_entries, size_t cb_offset_entries) {
	assert(ptd);
	
	if (indices == NULL) {
		assert(cb_size_entries == PVR_FULL_CODEBOOK);
		assert(cb_offset_entries == 0);
		indices = cb + PVR_CODEBOOK_SIZE_BYTES;
	} else {
		assert((cb_size_entries + cb_offset_entries) <= PVR_FULL_CODEBOOK);
	}
	
	ptd->compressed = true;
	
	memmove(ptd->codebook + cb_offset_entries, cb, cb_size_entries * PVR_CODEBOOK_ENTRY_SIZE_BYTES);
	
	ptd->tex_data = indices;
}

void ptdSetPalette(PvrTexDecoder *ptd, size_t size_colors, pxlABGR8888 *pal) {
	assert(ptd);
	assert(pal);
	assert(ptd->pixel_format == PT_PALETTE_4B || ptd->pixel_format == PT_PALETTE_8B);
	
	memmove(ptd->palette, pal,
		(ptd->pixel_format == PT_PALETTE_4B ? PVR_4B_PALETTE_SIZE : PVR_8B_PALETTE_SIZE) * sizeof(pxlABGR8888));
}

void ptdDecode(PvrTexDecoder *ptd) {
	assert(ptd);
	assert(ptd->w > 0);
	assert(ptd->w <= 1024);
	assert(ptd->h > 0);
	assert(ptd->h <= 1024);
	assert(ptd->mip_cnt);
	if (ptd->mips == 0)
		assert(ptd->mip_cnt == 1);
	else
		assert(ptd->mip_cnt > 2);
	assert(ptd->tex_data);
	
	unsigned size_pixels = CalcTextureSize(ptd->w, ptd->h, PT_PIXEL_OFFSET, ptd->mips, 0, 0);
	const void *src = ptd->tex_data;
	void *dec_data = NULL;
	
	if (ptd->compressed) {
		//For compressed mipmapped 4bpp textures, the number of indices is not
		//a multiple of the texture size. Round up the index count, and allocate
		//room for an extra vector worth of pixels
		dec_data = malloc(size_pixels * 2 + 16);
		unsigned vecarea = VectorArea(ptd->pixel_format);
		unsigned idxs = (size_pixels+vecarea-1) / vecarea;
		DecompressVQ(ptd->tex_data, idxs, ptd->codebook, 0, dec_data);
		src = dec_data;
	}
	
	FOR_EACH_DEC_MIP(ptd, i) {
		unsigned format = ptdGetConvertFormat(ptd, i);
		
		unsigned w = mw;
		//For 1x1 4bpp, we need to convert two pixels in a byte, so up the size
		if (format == PT_PALETTE_4B && mw == 1)
			w = 2;
		
		//Allocate buffer for unconverted mip level
		pxlABGR8888 *prev = malloc(w * mh * sizeof(pxlABGR8888));
		
		//Get pixel data for current mip level
		const void *pixels = src;
		if (ptd->mips) {
			//We already decompressed the image, so we always pass 0 for compression here
			unsigned ofs = MipMapOffset(ptd->pixel_format, 0, i);
			pixels += ofs;
		}
		
		//Convert image from pixels, storing in prev
		ConvertFromFormatToBGRA8888(pixels, format, ptd->palette, w, mh, prev);
		
		//For 4bpp, the pixel we need is stored as the second pixel of the two from the byte we converted
		if (format == PT_PALETTE_4B && mw == 0)
			prev[0] = prev[1];
		
		//Detwiddle if using twiddled format
		if (!ptd->stride)
			MakeDetwiddled32(prev, mw, mh);
		
		//Save deconverted image to pte
		SAFE_FREE(&ptd->result_mips[i]);
		ptd->result_mips[i] = prev;
	}
	
	SAFE_FREE(&dec_data);
}
