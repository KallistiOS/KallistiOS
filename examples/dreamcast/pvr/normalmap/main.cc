/*   
   bump.elf - an example showing how to use bumpmapping features
   Copyright (c) 2005 Fredrik Ehnbom

   This software is provided 'as-is', without any express or implied 
   warranty. In no event will the authors be held liable for any 
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any 
   purpose, including commercial applications, and to alter it and 
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you 
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product 
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and 
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source 
      distribution.
 */

/*
  This is a slightly altered version of the original bumpmapping
  example. This uses the new 2D normal map generation tool to
  create the bumpmap texture.

  It has been modified for formatting and for setting the viewable
  texture area.

  Thanks to Lobotomy for creating the texture and normal map along
  with updating this example to use the new normal map tool.
 */

#include <kos.h>
#include <png/png.h>
#include <stdlib.h>
#include <math.h>

#define TEXTURE_WIDTH	1024
#define TEXTURE_HEIGHT	1024

// T and Q defines the light source as polar coordinates where
// T is the elevation angle ranging from 0 to Pi/2 (90 degrees) and
// Q is the rotation angle ranging from 0 to Pi*2 (360 degrees).
// h is an intensity value specifying how much bumping should be done. 0 <= h <= 1
uint32 getBumpParameters(float T, float Q, float h)
{
	unsigned char Q2 = (unsigned char) ((Q / (2.0f * 3.1415927f))* 255);
	unsigned char h2 = (unsigned char) (h * 255);

	unsigned char k1 = 255 - h2;
	unsigned char k2 = (unsigned char) (h2 * fsin(T));
	unsigned char k3 = (unsigned char) (h2 * fcos(T));
	int oargb = k1 << 24 | k2 << 16 | k3 << 8 | Q2;

	return oargb;
}

void loadBumpmap(pvr_ptr_t dst)
{
	FILE *fp = fopen("/rd/bumpmap.raw", "rb");
	if (!fp) {
		printf("couldn't open romdisk...\n");
		exit(1);
	}
	unsigned char *data = (unsigned char*) malloc(TEXTURE_WIDTH*TEXTURE_HEIGHT*2);
	fread(data, 1, TEXTURE_WIDTH*TEXTURE_HEIGHT*2, fp);
	fclose(fp);
	pvr_txr_load(data, dst, TEXTURE_WIDTH*TEXTURE_HEIGHT*2);
	free(data);
}

void init_pvr()
{
	pvr_init_params_t params = {
		{PVR_BINSIZE_16, PVR_BINSIZE_0, /* Opaque polygons, Opaque modifiers */
		PVR_BINSIZE_16, PVR_BINSIZE_0, /* Transparent polygons, Transparent modifiers */
		PVR_BINSIZE_16}, /* Punchthru polygons */
		512 * 1024};
	pvr_init(&params);
}

int main(int argc, char **argv)
{
	maple_device_t *cont;
	cont_state_t *state;

	init_pvr();

	pvr_ptr_t dst = pvr_mem_malloc(TEXTURE_WIDTH * TEXTURE_HEIGHT * 2);
	loadBumpmap(dst);

	pvr_poly_cxt_t cxt;
	pvr_poly_hdr_t bumpHeader;

	// It appears that the bumpmap must be twiddled
	pvr_poly_cxt_txr(
		&cxt,
		PVR_LIST_OP_POLY,
		PVR_TXRFMT_BUMP | PVR_TXRFMT_TWIDDLED,
		TEXTURE_WIDTH,
		TEXTURE_HEIGHT,
		dst,
		PVR_FILTER_BILINEAR
	);

	// Really the only texture mode that makes sense
	// (The only one that works too ;))
	// Check pvr.h for more details
	cxt.txr.env = PVR_TXRENV_DECAL;
	cxt.gen.specular = true;
	pvr_poly_compile(&bumpHeader, &cxt);

	// ------------------------------------------------------------------------------------
	// this is not really necessary, but it's fun :)
	// The texture needs to be submitted in either the PT or the TR list
	// since the OP list will not do any blending operations needed to achive
	// multitexturing
	pvr_ptr_t texture;
	uint32 width;
	uint32 height;
	png_load_texture("/rd/texture.png", &texture, PNG_NO_ALPHA, &width, &height);
	pvr_poly_hdr_t textureHeader;
	pvr_poly_cxt_txr(
		&cxt,
		PVR_LIST_PT_POLY,
		PVR_TXRFMT_RGB565 | PVR_TXRFMT_TWIDDLED,
		width,
		height,
		texture,
		PVR_FILTER_BILINEAR
	);
	cxt.blend.src = PVR_BLEND_DESTCOLOR;
	cxt.blend.dst = PVR_BLEND_ZERO;
	pvr_poly_compile(&textureHeader, &cxt);

	// a little light
	pvr_ptr_t light;
	png_load_texture("/rd/light.png", &light, PNG_FULL_ALPHA, &width, &height);
	pvr_poly_hdr_t lightHeader;
	pvr_poly_cxt_txr(
		&cxt,
		PVR_LIST_TR_POLY,
		PVR_TXRFMT_ARGB4444 | PVR_TXRFMT_TWIDDLED,
		width,
		height,
		light,
		PVR_FILTER_BILINEAR
	);
	pvr_poly_compile(&lightHeader, &cxt);

	// the coordinates used for the quad
	pvr_vertex_t coords[4];

	coords[0].flags = PVR_CMD_VERTEX;
	coords[0].x = 80;
	coords[0].y = 480;
	coords[0].z = 1;
	coords[0].u = 0;
	coords[0].v = 1;

	// top left
	coords[1].flags = PVR_CMD_VERTEX;
	coords[1].x = 80;;
	coords[1].y = 0;
	coords[1].z = 1;
	coords[1].u = 0;
	coords[1].v = 0;

	// bottom right
	coords[2].flags = PVR_CMD_VERTEX;
	coords[2].x = 640-80;
	coords[2].y = 480;
	coords[2].z = 1;
	coords[2].u = 1;
	coords[2].v = 1;

	// top right
	coords[3].flags = PVR_CMD_VERTEX_EOL;
	coords[3].x = 640-80;
	coords[3].y = 0;
	coords[3].z = 1;
	coords[3].u = 1;
	coords[3].v = 0;

	while (1) {
		cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

		if(cont) {
			state = (cont_state_t *)maple_dev_status(cont);

			if (state && state->start)
				break;
		}

		float time = timer_ms_gettime64() / 1000.0f;
		float T = (0.5f + 0.5f * fsin(time * 0.33f)) * 3.141592f / 2;
		float Q = (0.5f + 0.5f * fsin(time)) * 3.141592f * 2;
		uint32 oargb = getBumpParameters(
			T,
			Q,
			0.5f
		);

		// Down below is just the submission of the data. Nothing
		// really interesting..
		pvr_wait_ready();
		pvr_scene_begin();
		pvr_list_begin(PVR_LIST_OP_POLY);

		pvr_prim(&bumpHeader, sizeof(pvr_poly_hdr_t));

		for (int i = 0; i < 4; i++) {
			coords[i].oargb = oargb;
			coords[i].argb = 0;
			pvr_prim(&coords[i], sizeof(pvr_vertex_t));
		}

		pvr_list_finish();

		// draw the texture
		pvr_list_begin(PVR_LIST_PT_POLY);
		pvr_prim(&textureHeader, sizeof(pvr_poly_hdr_t));

		for (int i = 0; i < 4; i++) {
			coords[i].oargb = 0;
			coords[i].argb = 0xffffffff;
			pvr_prim(&coords[i], sizeof(pvr_vertex_t));
		}

		pvr_list_finish();

		// draw the little light
		// Remember, this is just visual and has nothing to do with the
		// bump mapping
		pvr_list_begin(PVR_LIST_TR_POLY);

		pvr_prim(&lightHeader, sizeof(pvr_poly_hdr_t));

		// Ofcourse, Q and T should be calculated by the lights position
		// and not the other way around..
		float posx = 320 + fcos(Q) * ((3.141592f/2.0f) - T) * 150;
		float posy = 240 + fsin(Q) * ((3.141592f/2.0f) - T) * 150;
		int color = 0xffffffff;

		pvr_vertex_t *vert;
		pvr_dr_state_t state;
		pvr_dr_init(&state);

		// bottom left
		vert = pvr_dr_target(state);
		vert->flags = PVR_CMD_VERTEX;
		vert->x = posx - 8;
		vert->y = posy + 8;
		vert->z = 2;
		vert->u = 0;
		vert->v = 1;
		vert->argb = color;
		pvr_dr_commit(vert);

		// top left
		vert = pvr_dr_target(state);
		vert->flags = PVR_CMD_VERTEX;
		vert->x = posx - 8;
		vert->y = posy - 8;
		vert->z = 2;
		vert->u = 0;
		vert->v = 0;
		vert->argb = color;
		pvr_dr_commit(vert);

		// bottom right
		vert = pvr_dr_target(state);
		vert->flags = PVR_CMD_VERTEX;
		vert->x = posx + 8;
		vert->y = posy + 8;
		vert->z = 2;
		vert->u = 1;
		vert->v = 1;
		vert->argb = color;
		pvr_dr_commit(vert);

		// top right
		vert = pvr_dr_target(state);
		vert->flags = PVR_CMD_VERTEX_EOL;
		vert->x = posx + 8;
		vert->y = posy - 8;
		vert->z = 2;
		vert->u = 1;
		vert->v = 0;
		vert->argb = color;
		pvr_dr_commit(vert);

		pvr_dr_finish();
		pvr_list_finish();

		pvr_scene_finish();
 	}

	pvr_mem_free(dst);
	pvr_mem_free(light);

	return 0;
}
