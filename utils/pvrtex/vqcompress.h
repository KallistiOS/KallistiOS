#pragma once

#define VQC_MAX_CHANNELS	4

typedef enum {
	VQC_UINT8,
} vqcFormat;

typedef struct {
	vqcFormat format;
	unsigned channels;	//number of channels per pixel
	unsigned pix_per_cb;	//number of pixels per cb entry
	unsigned point_cnt;	//number of pixels / pix_per_cb
	unsigned cb_size;	//number of entries in cb
	
	unsigned dimensions;	//pix_per_cb * channels
	
	//channels can have different gammas (alpha could be 1.0, while RGB could be 2.2)
	float gamma[VQC_MAX_CHANNELS];
	
	size_t data_space;
	int *data;	//data to compress
} VQCompressor;

typedef struct {
	//Both codebook and indices may be NULL on error
	void *codebook;
	int *indices;
} vqcResults;

void vqcInit(VQCompressor *c, vqcFormat input_format, unsigned channels, unsigned pix_per_cb, unsigned cb_size);
void vqcAddPoints(VQCompressor *c, const void *src, size_t pixel_cnt);
void vqcSetChannelGamma(VQCompressor *c, unsigned channel, float val);
void vqcSetRGBAGamma(VQCompressor *c, float rgb, float alpha);
void vqcSetARGBGamma(VQCompressor *c, float rgb, float alpha);
vqcResults vqcCompress(VQCompressor *c, int quality);


