/*
 * KallistiOS ##version##
 *
 * examples/dreamcast/pvr/modifier_volume_zclip/pvr_zclip.h
 * Copyright (C) 2024 Twada
 *
 * This file contains the public API for commonly-used Z-clipping routines for
 * modifier volumes. 
 */

#include <kos.h>

/*
	Coordinates are received in (x, y, 1/z) and clipped in the range (1.0f >=z&>z0.0f).
	Since it uses a pair of single-precision transfer instructions, the data must be aligned in 8-byte increments.
	return : Bytes sent.
 */
extern int pvr_vertex_commit_zclip(pvr_vertex_t *vert, int size);
extern int pvr_vertex_commit_zclip_intensity(pvr_vertex_t *vert, int size);
extern int pvr_modifier_commit_zclip(pvr_mod_hdr_t *eol_header, pvr_modifier_vol_t *vol, int size);
