#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdarg.h>

#include "vqcompress.h"
#include "pixel.h"
#include "stb_image.h"
#include "stb_image_resize.h"
#include "pvr_texture.h"
#include "pvr_texture_encoder.h"
#include "mycommon.h"
#include "tddither.h"

/*
	Returns the pixel format for a given mipmap level
	YUV can change depending on level and if the texture is twiddled, this function
	returns the correct format.

	Passing 1 for miplevel will can be used to get the format for any level that
	isn't the 1x1 level
*/
unsigned pteGetConvertFormat(PvrTexEncoder *pte, int miplevel) {
	assert(pte);

	unsigned format = pte->pixel_format;
	if (format == PTE_YUV) {
		if (miplevel == 0 && pteHasMips(pte))
			return PTE_RGB565;
		if (pte->raw_is_twiddled)
			return PTE_YUV_TWID;
	}
	return format;
}


void pteInit(PvrTexEncoder *pte) {
	assert(pte);

	memset(pte, 0, sizeof(*pte));
	pte->mip_cnt = 0;
	pte->rgb_gamma = 1.0;
	pte->alpha_gamma = 1.0;
	pte->codebook_size = 0;
	pte->resize = PTE_FIX_NONE;
	pte->mipresize = PTE_FIX_MIP_NONE;
	pte->pixel_format = PTE_AUTO;
	pte->auto_small_vq = false;
	pte->edge_method = 0;
	pte->mip_shift_correction = true;
}

void pteFree(PvrTexEncoder *pte) {
	assert(pte);
	SAFE_FREE(&pte->pvr_codebook);
	SAFE_FREE(&pte->palette);
	SAFE_FREE(&pte->final_preview);
	SAFE_FREE(&pte->pvr_tex);
	SAFE_FREE(&pte->pvr_tex32);
	for(int i = 0; i < PVR_MAX_MIPMAPS; i++) {
		SAFE_FREE(&pte->src_imgs[i].pixels);
		SAFE_FREE(&pte->raw_mips[i]);
		SAFE_FREE(&pte->pvr_mips[i]);
		SAFE_FREE(&pte->preview_mips[i]);
	}
}

void pteLoadFromFiles(PvrTexEncoder *pte, const char **fnames, unsigned filecnt) {
	assert(filecnt < PVR_MAX_MIPMAPS);

	unsigned maxw = 0, maxh = 0;
	for(unsigned i = 0; i < filecnt ; i++) {
		pteImage *img = pte->src_imgs + i;
		img->w = img->h = 0;
		img->channels = 4;
		img->pixels = (void*)stbi_load(fnames[i], &img->w, &img->h, &img->channels, 4);
		ErrorExitOn(img->pixels == NULL,
			"Could not load image \"%s\", exiting\n", fnames[i]);
	
		if (filecnt > 1) {
			ErrorExitOn(!IsPow2(img->w) || !IsPow2(img->h), 
				"When using custom mipmaps, the size of all levels must be a power of two"
				" (resize is not supported). %s has a size of %ux%u\n", fnames[i], 
				img->w, img->h);
			ErrorExitOn(img->w != img->h, 
				"When using custom mipmaps, all levels must be square"
				" (resize is not supported). %s has a size of %ux%u\n", fnames[i], 
				img->w, img->h);
		}
	
		maxw = MAX(maxw, img->w);
		maxh = MAX(maxh, img->h);
	}
	pte->src_img_cnt = filecnt;
	pte->w = maxw;
	pte->h = maxh;
}

void pteMakeSquare(PvrTexEncoder *pte) {
	assert(pte);

	unsigned smaller = MIN(pte->h, pte->w);
	unsigned larger = MAX(pte->h, pte->w);

	switch(pte->mipresize) {
	case PTE_FIX_MIP_NONE:
		break;
	case PTE_FIX_MIP_MAX:
		pte->h = pte->w = MAX(pte->h, pte->w);
		break;
	case PTE_FIX_MIP_MIN:
		pte->h = pte->w = MIN(pte->h, pte->w);
		break;
	case PTE_FIX_MIP_NARROW_X2:
		pte->w = pte->h = MIN(smaller*2, larger);
		break;
	case PTE_FIX_MIP_NARROW_X4:
		pte->w = pte->h = MIN(smaller*4, larger);
		break;
	default:
		assert(0);
	}

	//Clamp width and height to PVR limits
	pte->w = CLAMP(8, pte->w, 1024);
	pte->h = CLAMP(8, pte->h, 1024);
}

int pteSetSize(PvrTexEncoder *pte) {
	assert(pte);
	assert(pte->w);
	assert(pte->h);

	switch(pte->resize) {
	case PTE_FIX_NONE:
		//Fail if width or height aren't valid
		if (pte->w > 1024 || pte->w < 8 || pte->h > 1024 || pte->h < 8)
			ErrorExit("Width and height must be between 8 and 1024, and no resize is set. Dimensions are %ix%i\n", pte->w, pte->h);
	
		if (!pteIsStrided(pte)) {
			if (!IsPow2(pte->w) || !IsPow2(pte->h))
				ErrorExit("Width and height must be a power of two for non-stride textures. Dimensions are %ix%i\n", pte->w, pte->h);
		} else {
			if (!IsValidStrideWidth(pte->w))
				ErrorExit("Width must be either 8, 16, or a multiple of 32 for stride textures. Width is %i\n", pte->w);
			//~ if (!pteIsPartial(pte))
				//~ assert(IsPow2(pte->h));
		}
		return 0;
		break;
	case PTE_FIX_UP:
		if (!pteIsStrided(pte)) {
			//Round width and height up to next higher power of two
			pte->w = RoundUpPow2(pte->w);
		} else {
			if (pte->w > 16)
				pte->w = (pte->w + 31) & ~0x1f;
			else if (pte->w > 8)
				pte->w = 16;
			else
				pte->w = 8;
		}
		pte->h = RoundUpPow2(pte->h);
		break;
	case PTE_FIX_DOWN:
		if (!pteIsStrided(pte)) {
			//Round width and height down to next higher power of two
			pte->w = RoundDownPow2(pte->w);
		} else {
			if (pte->w >= 32)
				pte->w = pte->w & ~0x1f;
			else if (pte->w >= 16)
				pte->w = 16;
			else
				pte->w = 8;
		}
		pte->h = RoundDownPow2(pte->h);
		break;
	case PTE_FIX_NEAREST: {
		if (!pteIsStrided(pte)) {
			//Round width to nearest power of two
			pte->w = SelectNearest(RoundDownPow2(pte->w), pte->w, RoundUpPow2(pte->w));
		
		} else {
			if (pte->w >= 24)
				pte->w = RoundNearest(pte->w, 32);
			else if (pte->w >= 12)
				pte->w = 16;
			else
				pte->w = 8;
		}
		//Round height to nearest power of two
		pte->h = SelectNearest(RoundDownPow2(pte->h), pte->h, RoundUpPow2(pte->h));
	} break;
	default:
		assert(0);
	}

	//Clamp width and height to PVR limits
	pte->w = CLAMP(8, pte->w, 1024);
	pte->h = CLAMP(8, pte->h, 1024);

	assert((pte->w % 4) == 0);

	pteLog(LOG_INFO, "Texture size: %ix%i\n", pte->w, pte->h);

	return 0;
}

void pteSetCompressed(PvrTexEncoder *pte, int codebook_size) {
	pte->codebook_size = codebook_size;
}

void pteConvertRawToTwiddled(PvrTexEncoder *pte) {
	assert(pte);
	assert(pte->stride == false);
	assert(pte->mip_cnt > 0);
	assert(pte->mip_cnt <= PVR_MAX_MIPMAPS);
	assert(IsPow2(pte->w));
	assert(IsPow2(pte->h));


	FOR_EACH_MIP(pte, i) {
		assert(pte->raw_mips[i]);
		MakeTwiddled32(pte->raw_mips[i], mw, mh);
	}

	pte->raw_is_twiddled = true;
}

void pteCombineABGRData(PvrTexEncoder *pte) {
	assert(pte);
	assert(pte->mip_cnt > 0);
	assert(pte->mip_cnt <= PVR_MAX_MIPMAPS);

	SMART_ALLOC(&pte->pvr_tex32, CalcTextureSize(pte->w, pte->h, (ptPixelFormat)PTE_RGB565, pteHasMips(pte), 0, 0) * 2);
	if (!pteIsCompressed(pte))
		SMART_ALLOC(&pte->pvr_tex, CalcTextureSize(pte->w, pte->h, (ptPixelFormat)PTE_RGB565, pteHasMips(pte), 0, 0));

	if (pteHasMips(pte)) {
		assert(pte->w == pte->h);
		FOR_EACH_MIP(pte, i) {
			assert(pte->raw_mips[i] != NULL);
		
			if (i == 0) {
				//For 1x1 mip, fill padding with pixel
				memcpy(pte->pvr_tex32 + 0, pte->raw_mips[i], mw * mh * sizeof(pxlABGR8888));
				memcpy(pte->pvr_tex32 + 1, pte->raw_mips[i], mw * mh * sizeof(pxlABGR8888));
				memcpy(pte->pvr_tex32 + 2, pte->raw_mips[i], mw * mh * sizeof(pxlABGR8888));
				memcpy(pte->pvr_tex32 + 3, pte->raw_mips[i], mw * mh * sizeof(pxlABGR8888));
			} else {
				memcpy(pte->pvr_tex32 + MipMapOffset(PT_PIXEL_OFFSET, 0, i), pte->raw_mips[i], mw * mh * sizeof(pxlABGR8888));
			}
		}
	} else {
		assert(pte->raw_mips[0] != NULL);
		memcpy(pte->pvr_tex32, pte->raw_mips[0], pte->w * pte->h * sizeof(pxlABGR8888));
	}
}

void pteGenerateUncompressed(PvrTexEncoder *pte) {
	assert(pte);
	assert(pte->mip_cnt > 0);
	assert(pte->mip_cnt <= PVR_MAX_MIPMAPS);

	//Handle first 4 pixels seperately in case we have mipped YUV. pteGetConvertFormat will figure out the correct format from pte
	ptConvertToTargetFormat(pte->pvr_tex32, 2, 2, pte->palette, pte->palette_size, pte->pvr_tex, pteGetConvertFormat(pte, 0));
	//Convert the rest
	ptConvertToTargetFormat(pte->pvr_tex32+4, CalcTextureSize(pte->w, pte->h, PT_PIXEL_OFFSET, pteHasMips(pte), 0, 0) - 4, 1,
		pte->palette, pte->palette_size, pte->pvr_tex+(int)(4*BytesPerPixel((ptPixelFormat)pte->pixel_format)), pteGetConvertFormat(pte, 1));

}

void pteDitherRaws(PvrTexEncoder *pte, float dither_amt) {
	assert(pte);
	assert(pte->mip_cnt > 0);
	assert(pte->mip_cnt <= PVR_MAX_MIPMAPS);
	assert(!pte->raw_is_twiddled);

	FOR_EACH_MIP(pte, i) {
		assert(pte->raw_mips[i] != NULL);
	
		pteDither((void*)pte->raw_mips[i], mw, mh, 4, dither_amt, pteGetFindNearest(pte->pixel_format), pte->palette, pte->palette_size, pte->raw_mips[i], PTE_ABGR8888);
	}
}


void pteGeneratePalette(PvrTexEncoder *pte) {
	assert(pte);
	assert(pte->mip_cnt > 0);
	assert(pte->mip_cnt <= PVR_MAX_MIPMAPS);
	assert(pte->palette == NULL);
	assert(pte->palette_size > 0);
	assert(pte->palette_size <= 256);

	VQCompressor vqc;
	vqcInit(&vqc, VQC_UINT8, 4, 1, pte->palette_size);
	vqcSetRGBAGamma(&vqc, pte->rgb_gamma, pte->alpha_gamma);

	//Add mipmaps to compressor input
	FOR_EACH_MIP(pte, i) {
		uint32_t pixelcnt = mw * mh;
		vqcAddPoints(&vqc, pte->raw_mips[i], pixelcnt);
	}

	//Do compression and save resulting palette
	vqcResults result = vqcCompress(&vqc, 8);
	assert(result.codebook);
	pte->palette = result.codebook;
	free(result.indices);

	//~ for(unsigned i = 0; i < 256; i++) {
		//~ printf("(%i, %i, %i) ", pte->palette[i].r, pte->palette[i].g, pte->palette[i].b, pte->palette[i].a);
	//~ }
}


typedef uint64_t CBVector;
typedef pxlABGR8888 PerfCodebook[PVR_FULL_CODEBOOK][16];
//If vec is in cb, returns idx, otherwise adds to cb and returns cb_used
static unsigned AddFindVector(PvrTexEncoder *pte, CBVector *cb, pxlABGR8888 *vec, unsigned vectorarea, unsigned cb_used, unsigned offset, unsigned format) {
	unsigned match = 0;

	CBVector vecconv = 0;
	assert(offset < vectorarea);
	ptConvertToTargetFormat(vec, vectorarea , 1, pte->palette, pte->palette_size, &vecconv, format);
	//~ printf("AddFindVector %016lx \n", vecconv);


	CBVector compare_mask = (CBVector)-1ll;
	unsigned bits_per_pixel = BytesPerPixel(format) * 8;
	compare_mask <<= offset * bits_per_pixel;
	CBVector compvec = vecconv;

	for(match = 0; match < cb_used; match++) {
		//Look for a matching entry in the perfect codebook
		//~ printf("Comp %016lx to %016lx (%016lx)\n", compvec, cb[match], compare_mask);
		if ((compvec & compare_mask) == (cb[match] & compare_mask))
			break;
	}
	assert(match < PVR_FULL_CODEBOOK);	//Don't overflow perfect CB
	if (match == cb_used) {
		//~ printf("Added %016lx at %u\n", vecconv, match);
		cb[match] = vecconv;
	}
	return match;
}

void pteCompress(PvrTexEncoder *pte) {
	assert(pte);
	assert(pte->mip_cnt > 0);
	assert(pte->mip_cnt <= PVR_MAX_MIPMAPS);
	assert(pte->codebook_size > 0);
	assert(pte->pvr_tex32 != NULL);
	if (!pteIsStrided(pte)) {
		assert(IsPow2(pte->w));
		assert(IsPow2(pte->h));
		assert(pte->raw_is_twiddled);
	}

	unsigned cbsize = pte->codebook_size;
	const unsigned vectorarea = VectorArea((ptPixelFormat)pte->pixel_format);

	pteLog(LOG_DEBUG, "Codebook size is %i\n", cbsize);

	//Calculate codebook size after taking out perfect mip codes
	unsigned gen_perfect_mip_vectors = 0;
	CBVector perfect_cb[PVR_FULL_CODEBOOK];
	assert(vectorarea <= 16);

	if (pteHasMips(pte)) {
		if ((pte->pixel_format == PTE_PALETTE_4B || pte->pixel_format == PTE_PALETTE_8B) && pte->perfect_mips < 2) {
			pteLog(LOG_DEBUG, "Need some perfect mips, so adding some\n");
			pte->perfect_mips = 2;
		} if (pte->pixel_format == PTE_YUV && pte->perfect_mips < 1) {
			pteLog(LOG_DEBUG, "Need some perfect mips, so adding some\n");
			pte->perfect_mips = 1;
		}
	} else {
		if (pte->perfect_mips != 0) {
			pteLog(LOG_WARNING, "Got --perfect-mips option, but not using any mipmaps.");
			pte->perfect_mips = 0;
		}
	}

	//Number of pixels we generate perfect mips for
	const unsigned perf_mip_size_pix = TotalMipSize(PT_PIXEL_OFFSET, 0, pte->perfect_mips);
	//Number of vectors we need to create. Round up for 4bpp mips
	const unsigned perfect_mip_idx = (perf_mip_size_pix + vectorarea - 1) / vectorarea;

	//Go through every perfect mip level and add each vector to a perfect codebook
	//We do not want to add duplicate vectors
	//Start at the highest level and work down, this way any incomplete levels, like the
	//1x1 mip level, can potentially reuse any vectors from higher mip levels if they are the same
	//If we start with the 1x1 mip and have junk in the unused part, it's unlikely we will
	//find a match in the higher levels
	for(int i = (perfect_mip_idx-1)*vectorarea; i >= 0; i -= vectorarea) {
		pxlABGR8888 *src = pte->pvr_tex32 + i;
		unsigned offset = 0;
		if (pteHasMips(pte) && i == 0) {
			//The start of the texture contains the smallest mip(s), which does not use the entire vector
			//offset is always 3 pixels
			offset = 3;
		}
	
		unsigned match = AddFindVector(pte, perfect_cb, src, vectorarea, gen_perfect_mip_vectors, offset, pteGetConvertFormat(pte, i));
		if (match >= gen_perfect_mip_vectors) {
			gen_perfect_mip_vectors++;
		}
	}

	pteLog(LOG_DEBUG, "Made %u perfect vectors\n", gen_perfect_mip_vectors);
	assert(gen_perfect_mip_vectors < pte->codebook_size);

	cbsize -= gen_perfect_mip_vectors;

	assert(cbsize <= PVR_FULL_CODEBOOK);
	assert(cbsize > 0);
	VQCompressor vqc;
	vqcInit(&vqc, VQC_UINT8, 4, vectorarea, cbsize);
	vqcSetRGBAGamma(&vqc, pte->rgb_gamma, pte->alpha_gamma);

	//Add uncompressed data
	const unsigned perfect_mip_pixels = perfect_mip_idx * vectorarea;
	const unsigned pxlcnt = CalcTextureSize(pte->w, pte->h, PT_PIXEL_OFFSET, pteHasMips(pte), 0, 0) - perfect_mip_pixels;
	const unsigned inperfveccnt = (pxlcnt+vectorarea-1) / vectorarea;

	if (pxlcnt % vectorarea) {
		//Because of the way compressed mipmapped 4bpp works, the bottom right index
		//is only partially used (2x4 is used out of 4x4), and the number of pixels
		//in the texture is not a multiple of the vector size.
		//~ printf("Have incomplete vector\n");
		assert(pteHasMips(pte) && pte->pixel_format == PTE_PALETTE_4B);
	
		unsigned npxlcnt = pxlcnt - pxlcnt % vectorarea;	//Round down to whole vector
		vqcAddPoints(&vqc, pte->pvr_tex32 + perfect_mip_pixels, npxlcnt);
		//~ inperfveccnt = npxlcnt+vectorarea-1) / vectorarea;
	
		//Expand the last 4x2 area into 4x4 by duplicating the 4x2 area
		pxlABGR8888 last[4*4];
		pxlABGR8888 *src = pte->pvr_tex32 + MipMapOffset((ptPixelFormat)pte->pixel_format, 0, pteTopMipLvl(pte)) + pte->w*pte->h - 8;
		memcpy(last, src, 8 * sizeof(pxlABGR8888));
		memcpy(last+8, src, 8 * sizeof(pxlABGR8888));
		vqcAddPoints(&vqc, last, 16);
	} else {
		//We can go ahead and add everything
		//~ printf("No incomplete vectors\n");
		assert(!(pteHasMips(pte) && pte->pixel_format == PTE_PALETTE_4B));
		vqcAddPoints(&vqc, pte->pvr_tex32 + perfect_mip_pixels, pxlcnt);
	}

	//Re-add some points to increase their weight
	//TODO We don't handle partial vectors which are required for 4BPP mipped, so turn off perfect mips for 4bpp mipped
	if (pte->high_weight_mips && pteHasMips(pte) && pte->pixel_format == PTE_PALETTE_4B) {
		pteLog(LOG_WARNING, "***Compressed mipmapped 4BPP does not currently support high weight mips***\nCreating texture without high weight mips\n");
		pte->high_weight_mips = 0;
	}
	const unsigned highlvl = pte->mip_cnt - pte->high_weight_mips;
	if (highlvl > 0 && highlvl < pte->mip_cnt) {
		unsigned high_start = perfect_mip_pixels;
		unsigned high_end = MipMapOffset(PT_PIXEL_OFFSET, 0, highlvl);
		pteLog(LOG_DEBUG, "High weight up and including to %u\n", 1u<<(highlvl-1));
		pteLog(LOG_DEBUG, "Re-adding bytes from %u to %u\n", high_start, high_end);
		if (high_end > high_start && high_end < pxlcnt)
			vqcAddPoints(&vqc, pte->pvr_tex32 + high_start, high_end - high_start);
		else
			pteLog(LOG_DEBUG, "Can't add high weight mips (start %u, end %u, pxlcnt %u)\n", high_start, high_end, pxlcnt);
	}

	//Do compression and save results
	pteLog(LOG_DEBUG, "Doing compression %u...\n", vqc.point_cnt); fflush(stdout);
	vqcResults result = vqcCompress(&vqc, 200);
	pteLog(LOG_DEBUG, "Done!\n"); fflush(stdout);
	assert(result.indices);
	assert(result.codebook);

	//Create PVR codebook
	SMART_ALLOC(&pte->pvr_codebook, PVR_CODEBOOK_SIZE_BYTES);
	ptConvertToTargetFormat(result.codebook, cbsize, vectorarea, pte->palette, pte->palette_size,
		pte->pvr_codebook + pte->pvr_idx_offset*8, pteGetConvertFormat(pte, 1));

	//Add any perfect CB vectors to end of generated CB
	unsigned perfectcbofs = pte->pvr_idx_offset + pte->codebook_size - gen_perfect_mip_vectors;
	void *cbend = pte->pvr_codebook + perfectcbofs * 8;
	memcpy(cbend, perfect_cb, gen_perfect_mip_vectors * 8);

	//Build up indices for texture
	SMART_ALLOC(&pte->pvr_tex, CalcTextureSize(pte->w, pte->h, (ptPixelFormat)pte->pixel_format, pteHasMips(pte), 1, PVR_CODEBOOK_SIZE_BYTES));
	uint8_t *texdst = pte->pvr_tex;

	//Add any perfect mips
	for(int i = 0; i < perfect_mip_idx; i++) {
		//Go though each vector and find the matching codebook entry
		pxlABGR8888 *src = pte->pvr_tex32 + i*vectorarea;
		unsigned offset = 0;
		if (pteHasMips(pte) && i == 0) {
			//The start of the texture contains the smallest mip(s), which does not use the entire vector
			//offset is always 3 pixels
			offset = 3;
		}
	
		unsigned format = pteGetConvertFormat(pte, i);
	
		unsigned match = AddFindVector(pte, perfect_cb, src, vectorarea, gen_perfect_mip_vectors, offset, format);
		assert(match < gen_perfect_mip_vectors);	//We should always find a match
		texdst[i] = match + perfectcbofs;
	}

	//Convert data from VQCompressor (int32 to uint8)
	for(int d = perfect_mip_idx, s = 0; s < inperfveccnt; d++, s++) {
		texdst[d] = result.indices[s] + pte->pvr_idx_offset;
	}

	free(result.indices);
}

static pteImage * pteHighestSrcMip(PvrTexEncoder *pte) {
	unsigned highest = 0;
	unsigned highestw = pte->src_imgs[0].w;

	for(unsigned i = 1; i < pte->src_img_cnt; i++) {
		if (pte->src_imgs[i].w > highestw) {
			highestw = pte->src_imgs[i].w;
			highest = i;
		}
	}

	return pte->src_imgs + highest;
}

//w is the width in pixels of the mip level to generate an image for
static pteImage pteGetShrinkLevel(PvrTexEncoder *pte, int w) {
	assert(pte);
	assert(pte->src_img_cnt > 0);
	assert(pte->src_imgs[0].pixels);

	unsigned level = MipLevels(w);

	if (pte->want_mips == PTE_MIP_QUALITY)
		level += 2;

	int desired_width = 1 << (level);

	//Default to first src_img, since it's known good
	pteImage best = pte->src_imgs[0];

	//If the desired width is for a level small enough that we have already created it,
	//use the existing level
	if (desired_width <= pte->w) {
		pteLog(LOG_DEBUG, "using existing mip %i (%i)\n", desired_width, level);
		best.w = best.h = desired_width;
		best.pixels = pte->raw_mips[level];
	}

	//Check rest of src_imgs for an exact match with w, this overrides everything
	for(unsigned i = 0; i < pte->src_img_cnt; i++) {
		pteImage *cur = pte->src_imgs + i;
		if (cur->w == w && cur->h == w) {
			//Have exact match
			pteLog(LOG_DEBUG, "Match\n");
			best = *cur;
			break;
		}
	}

	return best;
}

void pteGenerateMips(PvrTexEncoder *pte) {
	assert(pte);
	assert(!pte->raw_is_twiddled);		//can't generate if already twiddled, twiddle afterwards
	assert(pte->mip_cnt == 0);	//have no mips

	//Fix size if resizing is enabled
	pteMakeSquare(pte);

	ErrorExitOn(pteIsStrided(pte), "Mipmapped textures must be twiddled, but have stride parameter\n");
	ErrorExitOn(pte->w != pte->h, "Image must be square, but dimensions are (%ux%u)\n", pte->w, pte->h);
	ErrorExitOn(!IsPow2(pte->w) || !IsPow2(pte->h), "Height and width must be a power of two, but dimensions are (%ux%u)\n", pte->w, pte->h);

	assert(pte->w == pte->h);	//have square
	assert(IsPow2(pte->w));
	assert(IsPow2(pte->h));
	assert(pte->w >= 8);

	pte->mip_cnt = MipLevels(pte->w);

	assert(pte->mip_cnt > 0);
	assert(pte->mip_cnt <= 11);

	FOR_EACH_MIP_REV(pte, i) {
		//Allocate space for mip level
		unsigned pixelcnt = mw * mh;
		assert(pte->raw_mips[i] == NULL);	//Should have no raws yet
		SMART_ALLOC(&pte->raw_mips[i], pixelcnt * sizeof(pxlABGR8888));
	
		pteImage src = pteGetShrinkLevel(pte, mw);
		pteLog(LOG_INFO, "Making %ux%u mip from %ux%u image\n", mw, mh, src.w, src.h);
		if (src.w == mw && src.h == mh) {
			memcpy(pte->raw_mips[i], src.pixels, mw * mh * sizeof(pxlABGR8888));
		} else {
			float shift = 0;
			if (pte->mip_shift_correction)
				shift = -0.5 + (float)mw / pte->w / 2;
		
			stbir_resize_subpixel(src.pixels, src.w, src.h, 0,
				pte->raw_mips[i], mw, mh, 0,
				STBIR_TYPE_UINT8,	//format
				4,	//channels
				3,	//alpha
				0, //alpha flags (use default handling)
				pte->edge_method, pte->edge_method,
			
				STBIR_FILTER_DEFAULT, STBIR_FILTER_DEFAULT,
			
				STBIR_COLORSPACE_SRGB,
				//~ STBIR_COLORSPACE_LINEAR,
				NULL,	//alloc
				(float)mw / src.w, (float)mw / src.h,	//xscale, yscale,
				shift, shift	//xoffset, yoffset
				);
		}
	}
}


int pteGenerateRawFromSource(PvrTexEncoder *pte) {
	assert(pte);
	assert(pte->src_img_cnt >= 1);
	assert(pte->src_imgs[0].pixels);
	assert(pte->w >= 8);
	assert(pte->h >= 8);
	assert(pte->w <= 1024);
	assert(pte->h <= 1024);

	size_t tex_raw_size = pte->w * pte->h * sizeof(pxlABGR8888);
	SMART_ALLOC(&pte->raw_mips[0], tex_raw_size);

	//If no input mips, set raw_mips to one level, with one image from source data
	pteImage *h = pteHighestSrcMip(pte);
	if (h->w == pte->w && h->h == pte->h) {
		//If src size is equal to texture size, just copy the data over
		pteLog(LOG_INFO, "Source size matches texture size\n");
		memcpy(pte->raw_mips[0], h->pixels, tex_raw_size);
		pte->mip_cnt = 1;
	} else {
		//If src size is different to texture size, resize source to fit texture
		pteLog(LOG_INFO, "Source is getting resized from %ux%u to %ux%u\n", h->w, h->h, pte->w, pte->h);
		pte->mip_cnt = 1;
		stbir_resize(h->pixels, h->w, h->h, 0,
			pte->raw_mips[0], pte->w, pte->h, 0,
			STBIR_TYPE_UINT8,	//format
			4,	//channels
			3,	//alpha
			0, //alpha flags (use default handling)
			pte->edge_method, pte->edge_method,
			STBIR_FILTER_DEFAULT, STBIR_FILTER_DEFAULT,
			STBIR_COLORSPACE_SRGB,
			//~ STBIR_COLORSPACE_LINEAR,
			NULL	//alloc
			);
	}
	return 0;
}

void pteGeneratePreviews(PvrTexEncoder *pte) {
	assert(pte);
	assert(pte->pvr_tex);

	unsigned size_pixels = CalcTextureSize(pte->w, pte->h, PT_PIXEL_OFFSET, pteHasMips(pte), 0, 0);
	void *src = pte->pvr_tex;
	if (pteIsCompressed(pte)) {
		//For compressed mipmapped 4bpp textures, the number of indices is not
		//a multiple of the texture size. Round up the index count, and allocate
		//room for an extra vector worth of pixels
		src = malloc(size_pixels * 2 + 16);
		unsigned vecarea = VectorArea((ptPixelFormat)pte->pixel_format);
		unsigned idxs = (size_pixels+vecarea-1) / vecarea;
		DecompressVQ(pte->pvr_tex, idxs, pte->pvr_codebook, 0, src);
	}

	FOR_EACH_MIP(pte, i) {
		unsigned format = pteGetConvertFormat(pte, i);
	
		unsigned w = mw;
		//For 1x1 4bpp, we need to convert two pixels in a byte, so up the size
		if (format == PTE_PALETTE_4B && mw == 1)
			w = 2;
	
		//Allocate buffer for unconverted mip level
		pxlABGR8888 *prev = malloc(w * mh * sizeof(pxlABGR8888));
	
		//Get pixel data for current mip level
		void *pixels = src;
		if (pteHasMips(pte)) {
			//We already decompressed the image, so we always pass 0 for compression here
			unsigned ofs = MipMapOffset((ptPixelFormat)pte->pixel_format, 0, i);
			pixels += ofs;
		}
	
		//Convert image from pixels, storing in prev
		ConvertFromFormatToBGRA8888(pixels, format, pte->palette, w, mh, prev);
	
		//For 4bpp, the pixel we need is stored as the second pixel of the two from the byte we converted
		if (format == PTE_PALETTE_4B && mw == 0)
			prev[0] = prev[1];
	
		//Detwiddle if using twiddled format
		if (pte->raw_is_twiddled)
			MakeDetwiddled32(prev, mw, mh);
	
		//Save deconverted image to pte
		SAFE_FREE(&pte->preview_mips[i]);
		pte->preview_mips[i] = prev;
	}

	if (pteHasMips(pte)) {
		//Create preview image with combined mips
		assert(pte->final_preview == NULL);
		unsigned mp_w = pte->w * 1.5;
	
		pte->final_preview_w = mp_w;
		SMART_ALLOC(&pte->final_preview, pte->h * mp_w * sizeof(pxlABGR8888));
		pxlABGR8888 *mp = pte->final_preview;
		unsigned mipy = pte->h - 1;
	
		if (1) {
			//Create preview with all mips
			FOR_EACH_MIP(pte, i) {
				assert(pte->preview_mips[i]);
			
				//The highest mip level goes to the top left, the rest are on the right
				//For the non-top levels, we start at the bottom then go up
				unsigned mipx = pte->w;
				mipy -= mh;
				if (i == pteTopMipLvl(pte)) {
					mipy = 0;
					mipx = 0;
				}
			
				//Copy rows from preview_mips to final_preview
				for(unsigned y = 0; y < mh; y++) {
					memcpy(mp + (y + mipy)*mp_w + mipx, pte->preview_mips[i] + y*mw, mw * sizeof(pxlABGR8888));
				}
			
			
			}
		} else {
			//Create preview of top mip level
			int i = pteTopMipLvl(pte);
			for(unsigned y = 0; y < pte->h; y++) {
				memcpy(mp + y*mp_w, pte->preview_mips[i] + y*pte->w, pte->w * sizeof(pxlABGR8888));
			}
		}
	} else {
		//Create preview image without mips
		pte->final_preview_w = pte->w;
		SMART_ALLOC(&pte->final_preview, pte->h * pte->w * sizeof(pxlABGR8888));
	
		assert(pte->preview_mips[0]);
		memcpy(pte->final_preview, pte->preview_mips[0], pte->w * pte->h * sizeof(pxlABGR8888));
	}

	if (pteIsCompressed(pte)) {
		free(src);
	}
}

void pteConvertRawHeightToNormals(PvrTexEncoder *pte) {
	assert(pte);
	assert(!pte->raw_is_twiddled);

	FOR_EACH_MIP(pte, i) {
		assert(pte->raw_mips[i]);
	
		v3f *norms = malloc(mw * mw * sizeof(*norms));
	
		const pxlABGR8888 *src = pte->raw_mips[i];
	
		//Calculate normals from height map
		for(int y = 0; y < mh; y++) {
			for(int x = 0; x < mw; x++) {
				//TODO change this so that it copies the image to a larger temp buffer with the correct
				//edges, to avoid these per pixel checks and get STBIR_EDGE_ZERO working
			
				//Wrap around offsets
				unsigned l, r, u, d;
				if (pte->edge_method == STBIR_EDGE_WRAP) {
					l = x == 0 ? mw-1 : x-1;
					r = x == (mw-1) ? 0 : x+1;
					u = y == 0 ? mh-1 : y-1;
					d = y == (mh-1) ? 0 : y+1;
				} else if (pte->edge_method == STBIR_EDGE_CLAMP) {
					l = x == 0 ? x : x-1;
					r = x == (mw-1) ? x : x+1;
					u = y == 0 ? y : y-1;
					d = y == (mh-1) ? y : y+1;
				} else if (pte->edge_method == STBIR_EDGE_REFLECT) {
					l = x == 0 ? x+1 : x-1;
					r = x == (mw-1) ? x-1 : x+1;
					u = y == 0 ? y+1 : y-1;
					d = y == (mh-1) ? y-1 : y+1;
				} else {
					ErrorExit("Zero edge method not supported for height maps");
				}
			
				unsigned ofs = y*mw+x;
			
				norms[ofs].x = pxlU8toF(src[y*mw + l].r) - pxlU8toF(src[y*mw + r].r);
				norms[ofs].y = pxlU8toF(src[d*mw + x].r) - pxlU8toF(src[u*mw + x].r);
				norms[ofs].z = sqrtf(1 - norms[ofs].x*norms[ofs].x + norms[ofs].y*norms[ofs].y);
				//Gotten some weirdness on some heightmaps if we normalize here, so it's disabled for now
				//~ norms[ofs] = v3NormalizeS(norms[ofs]);
			}
		}
	
		//Replace raw_mips with normals
		for(int y = 0; y < mh; y++) {
			for(int x = 0; x < mw; x++) {
				unsigned ofs = y*mw+x;
				pte->raw_mips[i][ofs].r = pxlFtoU8B(norms[ofs].x);
				pte->raw_mips[i][ofs].g = pxlFtoU8B(norms[ofs].y);
				pte->raw_mips[i][ofs].b = pxlFtoU8B(norms[ofs].z);
				pte->raw_mips[i][ofs].a = 255;
			}
		}
	
	
		free(norms);
	}
}

void pteAutoSelectPixelFormat(PvrTexEncoder *pte) {
	assert(pte);
	assert(pte->src_img_cnt);

	unsigned clearpix = 0, halfpix = 0;

	for(int j = 0; j < pte->src_img_cnt; j++) {
		pteImage *img = pte->src_imgs + j;
	
		for(int i = 0; i < img->w * img->h; i++) {
			int a = img->pixels[i].a;
			if (a == 0)
				clearpix++;
			else if (a != 0xff)
				halfpix++;
		}
	}

	if (halfpix > 0)
		pte->pixel_format = PTE_ARGB4444;
	else if (clearpix > 0)
		pte->pixel_format = PTE_ARGB1555;
	else
		pte->pixel_format = pte->pixel_format == PTE_AUTO_YUV ? PTE_YUV : PTE_RGB565;
	pteLog(LOG_INFO, "Selected pixel format %s\n", ptGetPixelFormatString(pte->pixel_format));
}

void pteEncodeTexture(PvrTexEncoder *pte) {
	//Generate resized ABGR data from source image
	if (pte->want_mips) {
		pteLog(LOG_PROGRESS, "Generating mipmaps...\n");
		pteGenerateMips(pte);
	} else if (pte->src_img_cnt == 1) {
		pteGenerateRawFromSource(pte);
	} else {
		ErrorExit("Multiple source images have been specified, but mipmaps have not been requested\n");
	}

	//If the source image is a height map, convert it to a normal map
	if (pte->pixel_format == PTE_BUMP) {
		pteConvertRawHeightToNormals(pte);
		pte->pixel_format = PTE_NORMAL;
	}

	//Generate palette
	if (pte->pixel_format == PTE_PALETTE_4B || pte->pixel_format == PTE_PALETTE_8B) {
		if (pte->pixel_format == PTE_PALETTE_8B) {
			if (pte->palette_size == 0) {
				pte->palette_size = 256;
			} else if (pte->palette_size > 256) {
				ErrorExit("palette size must be 256 or less for 8bpp textures\n");
			}
		} else if (pte->pixel_format == PTE_PALETTE_4B) {
			if (pte->palette_size == 0) {
				pte->palette_size = 16;
			} else if (pte->palette_size > 16) {
				ErrorExit("palette size must be 16 or less for 4bpp textures\n");
			}
		}
		pteLog(LOG_PROGRESS, "Generating palette...\n");
		pteGeneratePalette(pte);
	}

	//Do dithering
	if (pte->dither && pte->pixel_format != PTE_YUV) {
		pteLog(LOG_PROGRESS, "Dithering...\n");
		pteDitherRaws(pte, pte->dither);
	}

	//Twiddle if texture is not strided
	if (!pte->stride) {
		pteLog(LOG_PROGRESS, "Twiddling...\n");
		pteConvertRawToTwiddled(pte);
	} else {
		//Stride textures cannot be these formats
		//Normal maps kill PVR if bilinear is used, and strided palettes can't be set
		if (pte->pixel_format == PTE_NORMAL || pte->pixel_format == PTE_PALETTE_4B || pte->pixel_format == PTE_PALETTE_8B)
			ErrorExit("Stride textures cannot be normal maps or palettized textures\n");
	}

	//Convert from internal ABGR8888 to final output format
	pteCombineABGRData(pte);
	if (pteIsCompressed(pte)) {
		pteLog(LOG_PROGRESS, "Compressing...\n");
		pteCompress(pte);
		pteLog(LOG_PROGRESS, "Compressed...\n");
	
		float uncompsize = CalcTextureSize(pte->w, pte->h, (ptPixelFormat)PTE_RGB565, pteHasMips(pte), 0, 0);
		float compsize = CalcTextureSize(pte->w, pte->h, (ptPixelFormat)pte->pixel_format, pteHasMips(pte), 1, pte->codebook_size*8);
		pteLog(LOG_INFO, "Compression ratio: %f\n", uncompsize / compsize);
	} else {
		pteLog(LOG_PROGRESS, "Converting as uncompressed...\n");
		pteGenerateUncompressed(pte);
	}
}
