#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "pvr_texture_encoder.h"
#include "file_common.h"
#include "file_pvr.h"

int fPvrSmallVQCodebookSize(int texsize_pixels, int mip) {
	if (texsize_pixels <= 16) {
		return 16;
	} else if (texsize_pixels <= 32) {
		return mip ? 64 : 32;
	} else if (texsize_pixels <= 64) {
		return mip ? 256 : 128;
	}
	return 256;
}

void fPvrWrite(const PvrTexEncoder *pte, const char *outfname) {
	assert(pte);
	assert(pte->pvr_tex);
	assert(outfname);
	
	FILE *f = fopen(outfname, "w");
	assert(f);
	
	//Write header
	unsigned chunksize = 16;
	
	unsigned pvrfmt = FILE_PVR_SQUARE;
	if (pteIsCompressed(pte)) {
		pvrfmt = FILE_PVR_VQ;
		unsigned cb_size = 2048;
		unsigned int idxcnt = pte->w * pte->h / 4;
		if (pteHasMips(pte))
			idxcnt = idxcnt * 4/3 + 1;
		
		if (pte->auto_small_vq) {
			//We only generate real small VQ textures when small_vq is set
			pvrfmt = FILE_PVR_SMALL_VQ;
			cb_size = pte->codebook_size * 8;
		}
		
		if (pteIsPalettized(pte))
			ErrorExit(".PVR format does not support compressed palettized textures\n");
		if (pte->w != pte->h)
			ErrorExit(".PVR format does not support non-square compressed textures\n");
		
		chunksize += idxcnt+cb_size;
	} else {
		chunksize += CalcTextureSize(pte->w, pte->h, pte->pixel_format, pteHasMips(pte), 0, 0);
		
		if (pte->pixel_format == PT_PALETTE_8B) {
			pvrfmt = FILE_PVR_8BPP;
		} else if (pte->pixel_format == PT_PALETTE_4B) {
			pvrfmt = FILE_PVR_4BPP;
		}
		
		//.PVR does not store first 4 padding bytes of uncompressed mipmapped texture
		if (pteHasMips(pte))
			chunksize -= 4;
		
		if (pte->w != pte->h) {
			pvrfmt = pteIsStrided(pte) ? FILE_PVR_RECT : FILE_PVR_RECT_TWID;
			assert(!pteHasMips(pte));
		}
	}
	
	if (pteHasMips(pte))
		pvrfmt += FILE_PVR_MIP_ADD;
	
	WriteFourCC("PVRT", f);
	Write32LE(chunksize, f);	//chunk size
	Write32LE(pvrfmt | pte->hw_pixel_format, f);	//pixel format, type
	Write16LE(pte->w, f);
	Write16LE(pte->h, f);
	
	WritePvrTexEncoder(pte, f, pte->auto_small_vq ? PTEW_FILE_PVR_SMALL_VQ : PTEW_NO_SMALL_VQ, 4);
	
	fclose(f);
	
	assert(chunksize == FileSize(outfname));
}

typedef struct {
	char fourcc[4];
	uint32_t len;
	uint32_t gbix;
	uint32_t pad;
	char data[0];
} TexturePvrGbix;

typedef struct {
	char fourcc[4];
	uint32_t len;
	uint32_t type;
	uint16_t w, h;
	char data[0];
} TexturePvrHeader;

enum {
	TEXPVR_SQR_TWID = 1,
	TEXPVR_SQR_TWID_MIP = 2,
	TEXPVR_VQ_TWID = 3,
	TEXPVR_VQ_TWID_MIP = 4,
	TEXPVR_8B_TWID = 5,
	TEXPVR_8B_TWID_MIP = 6,
	TEXPVR_4B_TWID = 7,
	TEXPVR_4B_TWID_MIP = 8,
	TEXPVR_RECT = 9,
	TEXPVR_RECT_TWID = 13,
	TEXPVR_SMALL_VQ_TWID = 16,
	TEXPVR_SMALL_VQ_TWID_MIP = 17,
	TEXPVR_SQR_TWID_MIP_B = 18,	
};

int fPvrLoad(const char *fname, PvrTexDecoder *dst) {
	assert(fname);
	assert(dst);
	
	void *data = NULL;
	size_t size = Slurp(fname, &data);
	
	if (size == 0 || data == 0)
		goto err_exit;
	
	const void *fileend = data + size;
	
	const TexturePvrHeader *pvrh = data;
	
	//Check file is long enough to at least have header
	if ((size < sizeof(TexturePvrHeader)) || ((data + pvrh->len) > fileend)) {
		pteLog(LOG_WARNING, ".PVR file appears invalid (incomplete file?)\n");
		goto err_exit;
	}
	
	//Check for GBIX chunk
	if (memcmp(pvrh->fourcc, "GBIX", 4) == 0) {
		const TexturePvrGbix *g = (const TexturePvrGbix *)pvrh;
		dst->gbix = g->gbix;
		
		pvrh = (TexturePvrHeader*)(data + pvrh->len + 8);
		if (((void*)pvrh + pvrh->len) > fileend) {
			pteLog(LOG_WARNING, ".PVR file appears invalid (incomplete file?)\n");
			goto err_exit;
		}
	}
	
	if (memcmp(pvrh->fourcc, "PVRT", 4) != 0) {
		pteLog(LOG_WARNING, ".PVR file appears invalid (bad fourcc)\n");
		goto err_exit;
	}
	
	if ((void*)pvrh + pvrh->len > fileend) {
		pteLog(LOG_WARNING, ".PVR file appears invalid (bad fourcc)\n");
		goto err_exit;
	}
	
	int mip = 0, cbsize = 0, stride = 0;
	switch((pvrh->type >> 8) & 0xff) {
	case TEXPVR_RECT:
		stride = 1;
		break;
	case TEXPVR_RECT_TWID:
	case TEXPVR_SQR_TWID:
	case TEXPVR_8B_TWID:
	case TEXPVR_4B_TWID:
		break;
	case TEXPVR_8B_TWID_MIP:
	case TEXPVR_4B_TWID_MIP:
	case TEXPVR_SQR_TWID_MIP:
	case 0x12:	//??? used by SA2
		mip = 1;
		break;
	case TEXPVR_VQ_TWID_MIP:
		mip = 1;
		//fallthrough
	case TEXPVR_VQ_TWID:
		cbsize = 256;
		break;
	case TEXPVR_SMALL_VQ_TWID_MIP:
		mip = 1;
		//fallthrough
	case TEXPVR_SMALL_VQ_TWID:
		cbsize = fPvrSmallVQCodebookSize(pvrh->w, mip);
		break;
	default:
		pteLog(LOG_WARNING, ".PVR file appears invalid (unsupported or bad type '%02x')\n", (pvrh->type >> 8) & 0xff);
		goto err_exit;
	}
	
	unsigned pixel_format = pvrh->type & 0xff;
	
	/*
		Headhunter seems to use 8bpp as a RGB555 format?
	*/
	
	if (pixel_format == PT_PALETTE_8B)
		pixel_format = PT_ARGB1555;
	
	if (pixel_format == PT_PALETTE_8B || pixel_format == PT_PALETTE_4B) {
		pteLog(LOG_WARNING, ".PVR loader currently doesn't support palettized textures\n");
		goto err_exit;
	}
	if (pixel_format > PT_PALETTE_8B) {
		pteLog(LOG_WARNING, ".PVR file appears invalid (bad pixel format '%02x')\n", pixel_format);
		goto err_exit;
	}
	
	ptdSetSize(dst, pvrh->w, pvrh->h, mip);
	ptdSetPixelFormat(dst, pixel_format);
	ptdSetStride(dst, stride);
	if (cbsize) {
		ptdSetCompressedSource(dst, pvrh->data + cbsize*PVR_CODEBOOK_ENTRY_SIZE_BYTES, pvrh->data, cbsize, 0);
	} else {
		/*
			Entirety of texture data does not always start at 
			pvrh->data, sometimes unused the padding for the 
			mipmap 1x1 bytes is incomplete, or extra is added. 
			Assuming the texture ends at the end of chunk, and 
			calculating the start from that, is more likely to 
			decode correctly.
		*/
		const void *pvrt_end = (void*)((uintptr_t)pvrh + pvrh->len + 8);
		size_t pvr_size = CalcTextureSize(pvrh->w, pvrh->h, pixel_format, mip, !!cbsize, cbsize);
		const void *img_start = pvrt_end - pvr_size;
		
		if (pvrt_end > fileend || img_start < (void*)pvrh) {
			pteLog(LOG_WARNING, ".PVR file appears invalid (incomplete file?)\n");
			goto err_exit;
		}
		ptdSetUncompressedSource(dst, img_start);
	}
	
	ptdDecode(dst);
	
	return 0;
	
err_exit:
	SAFE_FREE(&data);
	return 1;
}
