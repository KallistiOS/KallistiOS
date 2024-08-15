#include <kos.h>
#include <stdlib.h>

// 512 kb OP VERT BUF
#define OP_VERTBUF_SIZE (512*1024)

KOS_INIT_FLAGS(INIT_DEFAULT);

pvr_init_params_t pvr_params = {
{ PVR_BINSIZE_16, 0, 0, 0, 0 }, OP_VERTBUF_SIZE, 1, 0, 0, 3
};

uint8_t __attribute__((aligned(32))) op_buf[OP_VERTBUF_SIZE];

void draw_pvr_line(vec3f_t *v1, vec3f_t *v2, int width, int color, int which_list, pvr_poly_hdr_t *which_hdr);

int main(int argc, char **argv) {
	pvr_poly_hdr_t hdr;
	pvr_poly_cxt_t cxt;

	vec3f_t v1, v2;
	int r, g, b;
	int color;
	int width;
	int offset;

	vid_set_enabled(0);
	vid_set_mode(DM_640x480, PM_RGB565);
	pvr_init(&pvr_params);
	vid_set_enabled(1);

	pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
	pvr_poly_compile(&hdr, &cxt);

	offset = 0;

	while (true) {
		pvr_wait_ready();
		pvr_scene_begin();
		pvr_set_vertbuf(PVR_LIST_OP_POLY, op_buf, OP_VERTBUF_SIZE);

		offset = (offset + 5) % 360;

		for (int i=0;i<32;i++) {
			v1.x = rand() % 128;
			v1.y = rand() % 64;
			v1.z = 5;

			v2.x = 500 + (rand() % 96);
			v2.y = 400 + (rand() % 48);
			v2.z = 5;

			v1.x += offset;
			v1.y += offset;

			v2.x -= offset;
			v2.y -= offset;

			r = rand() % 255;
			g = rand() % 255;
			b = rand() % 255;

			color = PVR_PACK_COLOR(1.0, (float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f);

			width = (rand() % 5) + 1;

			draw_pvr_line(&v1, &v2, width, color, PVR_LIST_OP_POLY, &hdr);
		}

		pvr_scene_finish();
	}
}

void draw_pvr_line(vec3f_t *v1, vec3f_t *v2, int width, int color, int which_list, pvr_poly_hdr_t *which_hdr) {
	pvr_vertex_t __attribute__((aligned(32))) line_verts[4];
	pvr_vertex_t *vert = line_verts;
	float lx1,lx2,ly1,ly2,lz1,lz2;
	float x1,y1,x2,y2,z1,z2;

	for (int i=0;i<4;i++) {
		line_verts[i].flags = PVR_CMD_VERTEX;
		line_verts[i].argb = color;
		line_verts[i].oargb = 0;
	}
	line_verts[3].flags = PVR_CMD_VERTEX_EOL;

	lx1 = v1->x;
	ly1 = v1->y;
	lz1 = v1->z;

	lx2 = v2->x;
	ly2 = v2->y;
	lz2 = v2->z;

	if(lx1 <= lx2) {
		x1 = lx1;
		y1 = ly1;
		z1 = lz1;
		x2 = lx2;
		y2 = ly2;
		z2 = lz2;
	} else {
		x1 = lx2;
		y1 = ly2;
		z1 = lz2;
		x2 = lx1;
		y2 = ly1;
		z2 = lz1;
	}

	// https://devcry.heiho.net/html/2017/20170820-opengl-line-drawing.html
	float dx = x2 - x1;
	float dy = y2 - y1;

	float hlw_invmag = frsqrt((dx*dx) + (dy*dy)) * ((float)width*0.5f);
	float nx = -dy * hlw_invmag;
	float ny = dx * hlw_invmag;

	vert->x = x1 + nx;
	vert->y = y1 + ny;
	vert->z = z1;
	vert++;

	vert->x = x1 - nx;
	vert->y = y1 - ny;
	vert->z = z2;
	vert++;

	vert->x = x2 + nx;
	vert->y = y2 + ny;
	vert->z = z1;
	vert++;

	vert->x = x2 - nx;
	vert->y = y2 - ny;
	vert->z = z2;

	pvr_list_prim(which_list, which_hdr, sizeof(pvr_poly_hdr_t));
	pvr_list_prim(which_list, &line_verts, 4 * sizeof(pvr_vertex_t));
}