#include <stdio.h>
#include <assert.h>
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
		chunksize += CalcTextureSize(pte->w, pte->h, (ptPixelFormat)pte->pixel_format, pteHasMips(pte), 0, 0);
	
		if (pte->pixel_format == PTE_PALETTE_8B) {
			pvrfmt = FILE_PVR_8BPP;
		} else if (pte->pixel_format == PTE_PALETTE_4B) {
			pvrfmt = FILE_PVR_4BPP;
		}
	
		//.PVR does not store first 4 padding bytes of uncompressed mipmapped texture
		if (pteHasMips(pte))
			chunksize -= 4;
	
		if (pte->w != pte->h) {
			pvrfmt = FILE_PVR_RECT;
			assert(!pteHasMips(pte));
		}
	}

	if (pteHasMips(pte))
		pvrfmt += FILE_PVR_MIP_ADD;

	WriteFourCC("PVRT", f);
	Write32LE(chunksize, f);	//chunk size
	Write32LE(pvrfmt | pte->pixel_format, f);	//pixel format, type
	Write16LE(pte->w, f);
	Write16LE(pte->h, f);

	WritePvrTexEncoder(pte, f, pte->auto_small_vq ? PTEW_FILE_PVR_SMALL_VQ : PTEW_NO_SMALL_VQ, 4);

	fclose(f);

	assert(chunksize == FileSize(outfname));
}
