#include <stdio.h>
#include <assert.h>
#include "pvr_texture_encoder.h"

#define FILE_PVR_SQUARE		(1<<8)
#define FILE_PVR_SQUARE_MIP	(2<<8)
#define FILE_PVR_VQ		(3<<8)
#define FILE_PVR_VQ_MIP		(4<<8)
#define FILE_PVR_4BPP		(5<<8)	//Assumed
#define FILE_PVR_4BPP_MIP	(6<<8)	//Assumed
#define FILE_PVR_8BPP		(7<<8)
#define FILE_PVR_8BPP_MIP	(8<<8)	//Assumed
#define FILE_PVR_RECT		(9<<8)
#define FILE_PVR_RECT_MIP	(10<<8)	//Not supported by hardware, but implied
#define FILE_PVR_SMALL_VQ	(16<<8)
#define FILE_PVR_SMALL_VQ_MIP	(17<<8)

#define FILE_PVR_MIP_ADD	(1<<8)

void fPvrWrite(const PvrTexEncoder *td, const char *outfname);
int fPvrSmallVQCodebookSize(int texsize_pixels, int mip);
