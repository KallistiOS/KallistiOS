#include <assert.h>
#include <string.h>
#include "file_common.h"
#include "pvr_texture_encoder.h"

void CheckedFwrite(const void *data, size_t size, FILE *f) {
	int writeamt = fwrite(data, 1, size, f);
	if (writeamt != size) {
		perror("");
		ErrorExit("write error, wanted to write %i, but only wrote %i\n", size, writeamt);
	}
}

void WriteFourCC(const char *fourcc, FILE *f) {
	assert(strlen(fourcc) == 4);
	CheckedFwrite(fourcc, 4, f);
}
void Write8(unsigned int val, FILE *f) {
	char vb[1] = {val};
	CheckedFwrite(vb, 1, f);
}
void Write32LE(unsigned int val, FILE *f) {
	char vb[4] = {val, val >> 8, val >> 16, val >> 24};
	CheckedFwrite(vb, 4, f);
}
void Write16LE(unsigned int val, FILE *f) {
	char vb[2] = {val, val >> 8};
	CheckedFwrite(vb, 2, f);
}
void WritePadZero(size_t len, FILE *f) {
	static char paddingarea[64] = {0};
	
	assert(f);
	assert(len < sizeof(paddingarea));
	
	CheckedFwrite(&paddingarea, len, f);
}
void WritePvrTexEncoder(const PvrTexEncoder *pte, FILE *f, ptewSmallVQType svq, int mip_skip) {
	assert(pte);
	assert(pte->pvr_tex);
	assert(f);
	
	unsigned texsize = CalcTextureSize(pte->w, pte->h, pte->pixel_format, pteHasMips(pte), pteIsCompressed(pte), 0);
	
	if (pteIsCompressed(pte)) {
		assert(pte->pvr_codebook);

		//Write CB
		unsigned cbsize = pte->codebook_size * PVR_CODEBOOK_ENTRY_SIZE_BYTES;
		if (svq == PTEW_NO_SMALL_VQ)
			cbsize = PVR_CODEBOOK_SIZE_BYTES;
		pteLog(LOG_DEBUG, "Writing %u bytes for codebook\n", (unsigned)cbsize);
		CheckedFwrite(pte->pvr_codebook + pte->pvr_idx_offset * PVR_CODEBOOK_ENTRY_SIZE_BYTES, cbsize, f);
	}
	
	if (!pteIsCompressed(pte) && pteHasMips(pte)) {
		CheckedFwrite(pte->pvr_tex + mip_skip, texsize-mip_skip, f);
	} else {
		CheckedFwrite(pte->pvr_tex, texsize, f);
	}
}

int FileSize(const char *fname) {
	assert(fname);
	
	FILE *f = fopen(fname, "r");
	if (f == NULL)
		return -1;
	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	fclose(f);
	return size;
}

size_t Slurp(const char *fname, void **data) {
	assert(fname);
	
	FILE *f = fopen(fname, "r");
	if (f == NULL)
		return 0;
	
	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	
	fseek(f, 0, SEEK_SET);
	
	SMART_ALLOC(data, size);
	size_t readamt = fread(*data, 1, size, f);
	
	fclose(f);
	
	return readamt;
}

