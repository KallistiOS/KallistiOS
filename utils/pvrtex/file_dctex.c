#include <stdio.h>
#include <assert.h>
#include "pvr_texture_encoder.h"
#include "pvr_texture_decoder.h"
#include "file_common.h"
#include "file_dctex.h"

static int convert_size(int size) {
	if (size > 512)
		return 7;
	else if (size > 256)
		return 6;
	else if (size > 128)
		return 5;
	else if (size > 64)
		return 4;
	else if (size > 32)
		return 3;
	else if (size > 16)
		return 2;
	else if (size > 8)
		return 1;
	else
		return 0;
	
}

void fDtWrite(const PvrTexEncoder *pte, const char *outfname) {
	assert(pte);
	
	FILE *f = fopen(outfname, "w");
	assert(f);
	
	unsigned textype = 0;
	textype |= pteHasMips(pte) << FDT_MIPMAP_SHIFT;
	textype |= pteIsCompressed(pte) << FDT_VQ_SHIFT;
	textype |= pte->hw_pixel_format << FDT_PIXEL_FORMAT_SHIFT;
	textype |= !pte->raw_is_twiddled << FDT_NOT_TWIDDLED_SHIFT;
	
	//If the width is a power of two, we don't need the stride bit set
	textype |= (pteIsStrided(pte) && !IsPow2(pte->w)) << FDT_STRIDE_SHIFT;
	
	textype |= ((pte->w / 32) & FDT_STRIDE_VAL_MASK) << FDT_STRIDE_VAL_SHIFT;
	textype |= !IsPow2(pte->h) << FDT_PARTIAL_SHIFT;
	textype |= convert_size(pte->w) << FDT_WIDTH_SHIFT;
	textype |= convert_size(pte->h) << FDT_HEIGHT_SHIFT;
	
	//Include size of header
	unsigned origsize = 32 + CalcTextureSize(pte->w, pte->h, pte->pixel_format, pteHasMips(pte), pteIsCompressed(pte), pte->codebook_size * 8);
	unsigned size = ROUND_UP_POW2(origsize, 32);
	unsigned paddingamt = size - origsize;
	pteLog(LOG_DEBUG, "File size: %u orig + %u pad = %u total\n", origsize, paddingamt, size);
	
	WriteFourCC("DcTx", f);
	Write32LE(size, f);
	Write8(0, f);	//Version
	Write8(0, f);	//Header size is (32 bytes / 32 - 1) = 0
	Write8(pteIsCompressed(pte) ? pte->codebook_size - 1 : 0, f);
	Write8(pteIsPalettized(pte) ? pte->palette_size - 1 : 0, f);
	Write16LE(pte->w, f);
	Write16LE(pte->h, f);
	Write32LE(textype, f);
	Write32LE(0, f);
	Write32LE(0, f);
	Write32LE(0, f);
	
	WritePvrTexEncoder(pte, f, PTEW_FILE_DCTEX_SMALL_VQ, 0);
	
	//Pad to 32 bytes
	WritePadZero(paddingamt, f);
	fclose(f);
	
	//Validate resulting file
	unsigned resultsize = FileSize(outfname);
	ErrorExitOn(resultsize != size, "Size of file written for \"%s\" was incorrect. Expected file to be %u bytes, but result was %u bytes.\n", outfname, size, resultsize);
	
	f = fopen(outfname, "r");
	void *readbuff = malloc(size);
	if (fread(readbuff, size, 1, f) == 1) {
		if (!fDtValidateHeader(readbuff)) {
			pteLog(LOG_WARNING, "**Error validating output for .DT**\n");
		}
	} else {
		pteLog(LOG_WARNING, "**Error reading file during validation check for .DT output**\n");
	}
	fclose(f);
	free(readbuff);
}


int fDtLoad(const char *fname, PvrTexDecoder *dst) {
	assert(fname);
	assert(dst);
	
	void *data = NULL;
	size_t size = Slurp(fname, &data);
	
	if (size == 0 || data == 0)
		goto err_exit;
	
	fDtHeader *hdr = data;
	if (size < sizeof(*hdr) || fDtGetTotalSize(hdr) < size) {
		pteLog(LOG_WARNING, ".PVR file appears invalid (incomplete file?)\n");
		goto err_exit;
	}
	
	if (!fDtValidateHeader(hdr)) {
		pteLog(LOG_WARNING, ".DT file appears corrupt\n");
		goto err_exit;
	}
	
	ptdSetSize(dst, fDtGetWidth(hdr), fDtGetHeight(hdr), fDtIsMipmapped(hdr));
	ptdSetPixelFormat(dst, fDtGetPixelFormat(hdr));
	ptdSetStride(dst, fDtIsStrided(hdr));
	if (fDtIsCompressed(hdr)) {
		unsigned cbsize = fDtGetCodebookSizeBytes(hdr) / PVR_CODEBOOK_ENTRY_SIZE_BYTES;
		pteLog(LOG_WARNING, "CB size %u\n", cbsize);
		ptdSetCompressedSource(dst,
			fDtGetPvrTexData(hdr) + fDtGetCodebookSizeBytes(hdr),
			fDtGetPvrTexData(hdr),
			cbsize,
			PVR_FULL_CODEBOOK - cbsize);
	} else {
		ptdSetUncompressedSource(dst, fDtGetPvrTexData(hdr));
	}
	
	ptdDecode(dst);
	
	return 0;
	
err_exit:
	SAFE_FREE(&data);
	return 1;
	
}

