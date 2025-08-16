#pragma once

#include "pvr_texture.h"

typedef struct {
	unsigned w, h;
	unsigned mip_cnt;
	bool mips;
	bool compressed;
	bool stride;
	ptPixelFormat pixel_format;
	
	uint64_t codebook[PVR_FULL_CODEBOOK];
	pxlABGR8888 palette[PVR_8B_PALETTE_SIZE];
	
	//Used by .PVR read to return GBIX value
	uint32_t gbix;
	
	/*
		If uncompressed, this is raw PVR texture data
		
		If compressed, this is a pointer to the indices, the codebook 
		must be written to this struct
	*/
	const void *tex_data;
	
	
	pxlABGR8888 *result_mips[PVR_MAX_MIPMAPS];
} PvrTexDecoder;


void ptdInit(PvrTexDecoder *ptd);
void ptdFree(PvrTexDecoder *d);

void ptdSetSize(PvrTexDecoder *ptd, unsigned w, unsigned h, bool mips);
void ptdSetPixelFormat(PvrTexDecoder *ptd, ptPixelFormat pixel_format);
void ptdSetPalette(PvrTexDecoder *ptd, size_t size_colors, pxlABGR8888 *pal);
void ptdSetStride(PvrTexDecoder *ptd, bool stride);
void ptdSetUncompressedSource(PvrTexDecoder *ptd, const void *pixels);
void ptdSetCompressedSource(PvrTexDecoder *ptd, const void *indices, const void *cb, size_t cb_size_entries, size_t cb_offset_entries);

void ptdDecode(PvrTexDecoder *d);

#define FOR_EACH_DEC_MIP(ptd, mipidx) \
	for(int mipidx = 0, mw = (ptd)->mips ? 1 : (ptd)->w, mh = (ptd)->mips ? 1 : (ptd)->h; mipidx < (ptd)->mip_cnt; mipidx++, mw <<= 1, mh <<= 1)
