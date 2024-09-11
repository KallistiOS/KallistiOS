#pragma once

#include <stdio.h>
#include "pvr_texture_encoder.h"

typedef enum {
	PTEW_NO_SMALL_VQ,
	PTEW_FILE_PVR_SMALL_VQ,
	PTEW_FILE_DCTEX_SMALL_VQ,
} ptewSmallVQType;

void WriteFourCC(const char *fourcc, FILE *f);
void Write32LE(unsigned int val, FILE *f);
void Write16LE(unsigned int val, FILE *f);
void Write8(unsigned int val, FILE *f);
//If mip_pad_override is less than zero, add normal padding to uncompressed mipmapped textures
//If mip_pad_override is >= 0, use mip_pad_override as the number of padding bytes for uncompressed mipmapped textures
void WritePvrTexEncoder(const PvrTexEncoder *td, FILE *f, ptewSmallVQType svq, int mip_pad_override);
void WritePadZero(size_t len, FILE *f);

//Returns size of file, returns -1 if file does not exist
int FileSize(const char *fname);
