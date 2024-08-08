#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "vqcompress.h"
#include "elbg.h"
#include "mycommon.h"

void vqcInit(VQCompressor *c, vqcFormat input_format, unsigned channels, unsigned pix_per_cb, unsigned cb_size) {
	assert(c);

	memset(c, 0, sizeof(*c));
	c->format = input_format;
	c->channels = channels;
	c->pix_per_cb = pix_per_cb;
	c->cb_size = cb_size;
	c->dimensions = pix_per_cb * channels;
	for(int i = 0; i < VQC_MAX_CHANNELS; i++)
		c->gamma[i] = 1.0f;
}

/*
	The idea was to cale the pixel values so that we don't loose detail when doing integer gamma correction.
	Setting this too high can cause ELBG to select incorrect codebook entries.
	It seems not scaling at all results in the best quality.
*/
#define INT_SCALE	(255.0f)

void vqcAddPoints(VQCompressor *c, const void *src, size_t pixel_cnt) {
	assert(c);
	assert(src);
	assert(c->format == VQC_UINT8);
	assert((pixel_cnt % c->pix_per_cb) == 0);	//Make sure pixel_cnt is multiple of codebook entry size

	size_t point_cnt = pixel_cnt / c->pix_per_cb;

	//Ensure room for new pixels
	size_t space_needed = (c->point_cnt + point_cnt) * sizeof(int) * c->dimensions;
	if (c->data_space <= space_needed) {
		//At least 64KB, or the exact amount needed if we need more than that.
		size_t more = space_needed < 64*1024 ? 64*1024 : space_needed;
		void *newdata = realloc(c->data, c->data_space + more);
		assert(newdata);
		c->data = newdata;
	}

	//Convert source data to ints for ELBG
	const unsigned char *srcc = src;
	int *dst = c->data + c->point_cnt * c->dimensions;
	size_t elem_cnt = pixel_cnt * c->channels;
	unsigned curchannel = 0;
	for(size_t i = 0; i < elem_cnt; i++) {
		//Get source value and convert to floating point
		float v = srcc[i] / 255.0f;
	
		//Gamma correction
		v = pow(v, c->gamma[curchannel++]);
		if (curchannel >= c->channels)
			curchannel = 0;
	
		//Scale to fixed point
		dst[i] = v * INT_SCALE;
	}

	c->point_cnt += point_cnt;
}

void vqcSetChannelGamma(VQCompressor *c, unsigned channel, float val) {
	assert(c);
	assert(channel < c->channels);
	assert(val > 0);
	c->gamma[channel] = val;
}

void vqcSetRGBAGamma(VQCompressor *c, float rgb, float alpha) {
	assert(c);
	assert(c->channels == 3 || c->channels == 4);

	c->gamma[0] = c->gamma[1] = c->gamma[2] = rgb;
	if (c->channels == 4)
		c->gamma[3] = alpha;
}

void vqcSetARGBGamma(VQCompressor *c, float rgb, float alpha) {
	assert(c);
	assert(c->channels == 3 || c->channels == 4);

	if (c->channels == 4) {
		c->gamma[0] = alpha;
		c->gamma[1] = c->gamma[2] = c->gamma[3] = rgb;
	} else {
		c->gamma[0] = c->gamma[1] = c->gamma[2] = rgb;
	}
}

vqcResults vqcCompress(VQCompressor *c, int quality) {
	assert(c);
	assert(c->cb_size);
	assert(c->dimensions);
	assert(c->point_cnt > 0);
	assert(c->data);

	//Allocate indicies and int codebook result buffer
	vqcResults result;
	const size_t cb_elem_cnt = c->cb_size * c->dimensions;
	int *int_codebook = malloc(cb_elem_cnt * sizeof(int));
	result.indices = malloc(c->point_cnt * sizeof(int));
	assert(int_codebook);
	assert(result.indices);

	//Run ELBG to generate codebook
	struct ELBGContext *elbgcxt = 0;
	struct AVLFG randcxt;
	av_lfg_init(&randcxt, 1);
	int errval = avpriv_elbg_do(&elbgcxt, c->data, c->dimensions, c->point_cnt, int_codebook, c->cb_size, quality, result.indices, &randcxt, 0);
	assert(errval == 0);
	avpriv_elbg_free(&elbgcxt);

	//Convert int_codebook to input_format
	assert(c->format == VQC_UINT8);
	char *dst = result.codebook = malloc(c->cb_size * c->dimensions);
	assert(dst);
	unsigned curchannel = 0;

	//Invert gamma values
	float invgamma[VQC_MAX_CHANNELS];
	for(int i = 0; i < c->channels; i++) {
		invgamma[i] = 1.0f / c->gamma[i];
	}

	for(size_t i = 0; i < cb_elem_cnt; i++) {
		//Get source value and convert to floating point
		float v = int_codebook[i] / INT_SCALE;
	
		//Undo gamma correction
		v = pow(v, invgamma[curchannel++]);
		if (curchannel >= c->channels)
			curchannel = 0;
	
		//Scale to fixed point
		dst[i] = v * 255.0f;
	}

	//Clean up
	free(int_codebook);
	SAFE_FREE(&c->data);
	c->data_space = 0;

	return result;
}


