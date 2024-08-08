#include <stdio.h>
#include <assert.h>
#include "pvr_texture_encoder.h"
#include "file_common.h"

void fTexWrite(const PvrTexEncoder *td, const char *outfname);
void fTexWritePalette(const PvrTexEncoder *td, const char *outfname);
void fTexWritePaletteAppendPal(const PvrTexEncoder *td, const char *outfname);
