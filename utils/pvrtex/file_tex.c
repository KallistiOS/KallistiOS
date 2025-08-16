#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "pvr_texture_encoder.h"
#include "file_common.h"
#include "file_tex.h"

void fTexWrite(const PvrTexEncoder *pte, const char *outfname) {
	assert(pte);
	
	FILE *f = fopen(outfname, "w");
	assert(f);
	
	unsigned textype = 0;
	textype |= pteHasMips(pte) ? (1<<31) : 0;
	textype |= pteIsCompressed(pte) ? (1<<30) : 0;
	textype |= pte->hw_pixel_format << 27;
	textype |= !pte->raw_is_twiddled << 26;
	textype |= pteIsStrided(pte) ? (1<<25) : 0;
	textype |= (pte->w / 32) & 0x1f;
	
	//Size does not include size of header
	unsigned origsize = CalcTextureSize(pte->w, pte->h, pte->pixel_format, pteHasMips(pte), pteIsCompressed(pte), PVR_CODEBOOK_SIZE_BYTES);
	unsigned size = ROUND_UP_POW2(origsize, 32);
	unsigned paddingamt = size - origsize;
	pteLog(LOG_DEBUG, "File DTEX size: (%u + %u) %u\n", origsize, paddingamt, size);
	
	WriteFourCC("DTEX", f);
	Write16LE(RoundUpPow2(pte->w), f);
	Write16LE(pte->h, f);
	Write32LE(textype, f);
	Write32LE(size, f);
	
	WritePvrTexEncoder(pte, f, PTEW_NO_SMALL_VQ, 0);
	
	//Pad to 32 bytes
	static const unsigned char padding[32] = {0};
	pteLog(LOG_DEBUG, "Padding %u\n", paddingamt);
	int ret = fwrite(padding, 1, paddingamt, f);
	assert(ret == paddingamt);
	
	fclose(f);
}

void fTexWritePaletteAppendPal(const PvrTexEncoder *pte, const char *outfname) {
	char palette_name[1024];
	size_t namelen = strlen(outfname);
	assert(namelen < (sizeof(palette_name) - 5));
	memcpy(palette_name, outfname, namelen);
	memcpy(palette_name + namelen, ".pal", 4);
	palette_name[namelen + 4] = '\0';
	
	fTexWritePalette(pte, palette_name);
}

void fTexWritePalette(const PvrTexEncoder *pte, const char *outfname) {
	assert(pte);
	assert(pte->palette);
	assert(pte->palette_size > 0);
	assert(pte->palette_size <= 256);
	
	pteLog(LOG_COMPLETION, "Writing .PAL to \"%s\"...\n", outfname);
	
	FILE *f = fopen(outfname, "w");
	assert(f);
	
	WriteFourCC("DPAL", f);
	Write32LE(pte->palette_size, f);
	
	for(unsigned i = 0; i < pte->palette_size; i++) {
		Write32LE(pxlConvertABGR8888toARGB8888(pte->palette[i]).argb, f);
	}
	
	fclose(f);
	
}
