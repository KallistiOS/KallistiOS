/*
 * KallistiOS ##version##
 *
 * examples/dreamcast/pvr/pvrline/pvrline.c
 * Copyright (C) 2024 Jason Martin
 *
 * This example demonstrates the use of the PVR to draw lines with quads
 * (as triangle strips).
 * It also demonstrates the use of pvr_list_prim, interleaving the drawing of
 * OPAQUE and TRANSPARENT polygons.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <kos.h>

/* 512 kb vertex buffer size */
#define VERTBUF_SIZE (512*1024)

KOS_INIT_FLAGS(INIT_DEFAULT);

/* enable OP and TR lists */
pvr_init_params_t pvr_params = {
{ PVR_BINSIZE_16, 0, PVR_BINSIZE_16, 0, 0 }, VERTBUF_SIZE, 1, 0, 0, 3, 0
};

uint8_t __attribute__((aligned(32))) op_buf[VERTBUF_SIZE];
uint8_t __attribute__((aligned(32))) tr_buf[VERTBUF_SIZE];

/*
 * given a vec3f_t representing the screen-space start of a line (x,y,z),
 * a vec3f_t representing the screen-space end of a line (x,y,z),
 * a color, a width, a pvr polygon list type and a pvr polygon header
 * uses the PVR to draw a line with a triangle strip consisting of 4 vertices
 * representing a quad
*/
void draw_pvr_line(vec3f_t *v1, vec3f_t *v2, float width, int color,
	int which_list, pvr_poly_hdr_t *which_hdr);

int main(int argc, char **argv)
{
	pvr_poly_hdr_t op_hdr, tr_hdr;
	pvr_poly_cxt_t op_cxt, tr_cxt;

	/* start and end points of a line in screen-space */
	vec3f_t v1, v2;
	/* red, green, blue and alpha color components */
	int r, g, b, a;
	/* packed color */
	int color;
	/* line width */
	int width;
	/* moving offset added or subtracted to x and y components of endpoints */
	int offset;
	/* number of lines to draw per frame */
	int linecount = 1;

	maple_device_t *controller;
	cont_state_t *cont;

	printf("---KallistiOS PVR Line-drawing Example---\n");
	printf("Press DPAD UP to increase line count\n\t(up to a maximum of 1536"
		" lines).\n");
	printf("Press DPAD DOWN to decrease line count\n\t(down to a minimum of 1"
		" line).\n");
	printf("Press A to reset line count to 1.\n");
	printf("Press Start to exit.\n");

	srand(time(NULL));

	vid_set_enabled(0);
	vid_set_mode(DM_640x480, PM_RGB565);
	pvr_init(&pvr_params);
	vid_set_enabled(1);

	/* polygon header setup, OP lines */
	pvr_poly_cxt_col(&op_cxt, PVR_LIST_OP_POLY);
	pvr_poly_compile(&op_hdr, &op_cxt);

	/* polygon header setup, TR lines */
	pvr_poly_cxt_col(&tr_cxt, PVR_LIST_TR_POLY);
	pvr_poly_compile(&tr_hdr, &tr_cxt);

	offset = 0;

	while (true) {
		controller = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
		if (controller) {
			cont = maple_dev_status(controller);
			if (cont->buttons & CONT_DPAD_UP) {
				if (linecount < 1536) {
					linecount += 1;
				}
			} else if (cont->buttons & CONT_DPAD_DOWN) {
				if (linecount > 1) {
					linecount -= 1;
				}
			} else if (cont->buttons & CONT_A) {
				linecount = 1;
			} else if (cont->buttons & CONT_START) {
				break;
			}
		}

		pvr_wait_ready();
		pvr_scene_begin();

		/* set vertex buffers for pvr_list_prim use */
		pvr_set_vertbuf(PVR_LIST_OP_POLY, op_buf, VERTBUF_SIZE);
		pvr_set_vertbuf(PVR_LIST_TR_POLY, tr_buf, VERTBUF_SIZE);

		/*
		 * incrementing offset added to endpoints
		 * make endpoints occasionally move to other side of screen
		 * from where they start
		 * force swaps in the line drawing routine
		 */
		offset = (offset + 5) % 360;

		for (int i = 0; i < linecount; i++) {
			v1.x = rand() % 128;
			v1.y = rand() % 64;
			v1.z = 5;

			v2.x = 500 + (rand() % 96);
			v2.y = 400 + (rand() % 48);
			v2.z = 5;

			/*
			 * add an offset in the range [0,359] to v1 x and y
			 * x stays within [0,487]
			 * y stays within [0,422]
			 */
			v1.x += offset;
			v1.y += offset;

			/*
			 * subtract an offset in the range [0,359] from v2 x and y
			 * x stays within [141,595]
			 * y stays within [40,447]
			 */
			v2.x -= offset;
			v2.y -= offset;

			/*
			 * the above offset can move v2 to the left of v1
			 * this demonstrates that lines will always render correctly
			 * regardless of vertex order
			 */

			/* generate a random RGBA color */
			r = rand() % 256;
			g = rand() % 256;
			b = rand() % 256;
			a = rand() % 256;

			color = PVR_PACK_COLOR((float)a / 255.0f,
				(float)r / 255.0f,
				(float)g / 255.0f,
				(float)b / 255.0f);

			width = (rand() % 5) + 1;

			/* interleaved use of PVR polygon list types */
			if (a == 255) {
				/*
				 * when alpha is fully opaque
				 * use the OP list
				 */
				draw_pvr_line(&v1, &v2, width, color,
					PVR_LIST_OP_POLY, &op_hdr);
			} else {
				/*
				 * when alpha is transparent at all
				 * use the TR list
				 */
				draw_pvr_line(&v1, &v2, width, color,
					PVR_LIST_TR_POLY, &tr_hdr);
			}
		}

		pvr_scene_finish();
	}
}

void draw_pvr_line(vec3f_t *v1, vec3f_t *v2, float width, int color,
	int which_list, pvr_poly_hdr_t *which_hdr)
{
	pvr_vertex_t __attribute__((aligned(32))) line_verts[4];
	pvr_vertex_t *vert = line_verts;

	vec3f_t *ov1;
	vec3f_t *ov2;

	for (int i=0;i<4;i++) {
		line_verts[i].flags = PVR_CMD_VERTEX;
		line_verts[i].argb = color;
		line_verts[i].oargb = 0;
	}
	line_verts[3].flags = PVR_CMD_VERTEX_EOL;

	/*
	 * when first vertex is to the left of or vertical with second vertex
	 *  they are already ordered
	 * otherwise
	 *  swap endpoints
	 */
	if(v1->x <= v2->x) {
		ov1 = v1;
		ov2 = v2;
	} else {
		ov1 = v2;
		ov2 = v1;
	}

	/*
	 * https://devcry.heiho.net/html/2017/20170820-opengl-line-drawing.html
	 *
	 * get the normal to the line segment running from v1 to v2
	 * use normal to draw a quad covering the actual line segment
	 */
	float dx = ov2->x - ov1->x;
	float dy = ov2->y - ov1->y;

	/*
	 * use the fast reciprocal square root function provided by KOS
	 * to get inverse of the magnitude of the normal
	 * multiply by half of the line width
	 * this scales the normal, making a quad with the requested line width
	 */
	float inverse_magnitude = frsqrt((dx*dx) + (dy*dy)) *
		((float)width*0.5f);
	float nx = -dy * inverse_magnitude;
	float ny = dx * inverse_magnitude;

	/* normal offset "down" from the first endpoint */
	vert->x = ov1->x + nx;
	vert->y = ov1->y + ny;
	vert->z = ov1->z;
	vert++;

	/* normal offset "up" from the first endpoint */
	vert->x = ov1->x - nx;
	vert->y = ov1->y - ny;
	vert->z = ov2->z;
	vert++;

	/* normal offset "down" from the second endpoint */
	vert->x = ov2->x + nx;
	vert->y = ov2->y + ny;
	vert->z = ov1->z;
	vert++;

	/* normal offset "up" from the second endpoint */
	vert->x = ov2->x - nx;
	vert->y = ov2->y - ny;
	vert->z = ov2->z;

	/* submit the poly header and vertices to requested list */
	pvr_list_prim(which_list, which_hdr, sizeof(pvr_poly_hdr_t));
	pvr_list_prim(which_list, &line_verts, 4 * sizeof(pvr_vertex_t));
}
