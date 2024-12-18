#include "pvr_zclip.h"

static void vert_commit(pvr_vertex_t *dest, pvr_vertex_t *src, int eos)
{
	asm volatile(
		"fschg\n\t"
		"add	#14, %[e]\n\t"
		"fmov	@%[s]+, dr0\n\t"
		"shld	%[shift], %[e]\n\t"
		"fmov	@%[s]+, dr2\n\t"
		"lds	%[e], fpul\n\t"
		"fmov	@%[s]+, dr4\n\t"
		"add	#32, %[d]\n\t"
		"fmov	@%[s]+, dr6\n\t"
		"fsts	fpul, fr0\n\t"
		"fmov	dr6,@-%[d]\n\t"
		"fmov	dr4,@-%[d]\n\t"
		"fmov	dr2,@-%[d]\n\t"
		"fmov	dr0,@-%[d]\n\t"
		"pref	@%[d]\n\t"
		"fschg\n"
		: [d] "+&r"(dest), [s] "+&r"(src), [e] "+r"(eos)
		: [shift] "r"(28)
		: "fpul", "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7");
}

static void inter_vert_commit(pvr_vertex_t *dest, pvr_vertex_t *inside, pvr_vertex_t *outside, int eos)
{
	unsigned int in_rb;
	unsigned int in_ag;
	unsigned int out_rb;
	unsigned int out_ag;
	asm volatile(
		"fschg\n\t"

		"fmov.d	@%[i]+, dr2\n\t"
		"fmov.d @%[i]+, dr4\n\t"
		"fmov.d	@%[o]+, dr6\n\t"
		"fmov.d @%[o]+, dr8\n\t"

		"fcmp/gt fr5, fr9\n\t"
		"add	#32, %[d]\n\t"
		"fmul	fr5, fr5\n\t"
		"mov	#28, %[rb0]\n\t"
		"fsrra	fr5\n\t"
		"add	#14, %[e]\n\t"
		"fmul	fr9, fr9\n\t"
		"shld	%[rb0], %[e]\n\t"
		"fsrra	fr9\n\t"
		"lds	%[e], fpul\n\t"
		"bt/s	1f\n\t"
		"fsts	fpul, fr2\n\t"
		"fneg	fr9\n\t"
		"1:\n\t"

		"fmul	fr5, fr3\n\t"
		"fmul	fr5, fr4\n\t"
		"fmul	fr9, fr7\n\t"
		"fmul	fr9, fr8\n\t"

		"fneg	fr9\n\t"
		"fadd	fr5, fr9\n\t"
		"fldi1	fr0\n\t"
		"fmul	fr9, fr9\n\t"
		"fneg	fr0\n\t"
		"fsrra	fr9\n\t"
		"fadd	fr5, fr0\n\t"
		"fmul	fr9, fr0\n\t"

		"fsub	fr3, fr7\n\t"
		"fsub	fr4, fr8\n\t"
		"fmac	fr0, fr7, fr3\n\t"
		"fmac	fr0, fr8, fr4\n\t"
		"fldi1	fr5\n\t"

		"fmov.d	@%[i]+, dr6\n\t"
		"fmov.d @%[o]+, dr8\n\t"
		"fsub	fr6, fr8\n\t"
		"fsub	fr7, fr9\n\t"
		"fmac	fr0, fr8, fr6\n\t"
		"fmac	fr0, fr9, fr7\n\t"

		"mov	#-1, %[e]\n\t"
		"extu.b	%[e], %[e]\n\t"
		"lds	%[e], fpul\n\t"
		"float	fpul, fr1\n\t"
		"fmul	fr0, fr1\n\t"
		"ftrc	fr1, fpul\n\t"
		"sts	fpul, %[e]\n\t"

		"mov.l	@(4, %[i]), %[rb0]\n\t"
		"mov	%[rb0], %[ag0]\n\t"
		"and	%[m], %[rb0]\n\t"
		"shlr8	%[ag0]\n\t"
		"mov.l	@(4, %[o]), %[rb1]\n\t"
		"and	%[m], %[ag0]\n\t"
		"mov	%[rb1], %[ag1]\n\t"
		"and	%[m], %[rb1]\n\t"
		"shlr8	%[ag1]\n\t"
		"and	%[m], %[ag1]\n\t"

		"sub	%[ag0], %[ag1]\n\t"
		"mul.l	%[e], %[ag1]\n\t"
		"sub	%[rb0], %[rb1]\n\t"
		"sts	macl, %[ag1]\n\t"
		"shlr8	%[ag1]\n\t"
		"add	%[ag1], %[ag0]\n\t"
		"mul.l	%[e], %[rb1]\n\t"
		"and	%[m], %[ag0]\n\t"
		"shll8	%[ag0]\n\t"
		"sts	macl, %[rb1]\n\t"
		"shlr8	%[rb1]\n\t"
		"add	%[rb1], %[rb0]\n\t"
		"and	%[m], %[rb0]\n\t"
		"or		%[ag0], %[rb0]\n"
		"mov.l	%[rb0], @-%[d]\n\t"

		"mov.l	@%[i], %[rb0]\n\t"
		"mov	%[rb0], %[ag0]\n\t"
		"and	%[m], %[rb0]\n\t"
		"shlr8	%[ag0]\n\t"
		"mov.l	@%[o], %[rb1]\n\t"
		"and	%[m], %[ag0]\n\t"
		"mov	%[rb1], %[ag1]\n\t"
		"and	%[m], %[rb1]\n\t"
		"shlr8	%[ag1]\n\t"
		"and	%[m], %[ag1]\n\t"

		"sub	%[ag0], %[ag1]\n\t"
		"mul.l	%[e], %[ag1]\n\t"
		"sub	%[rb0], %[rb1]\n\t"
		"sts	macl, %[ag1]\n\t"
		"shlr8	%[ag1]\n\t"
		"add	%[ag1], %[ag0]\n\t"
		"mul.l	%[e], %[rb1]\n\t"
		"and	%[m], %[ag0]\n\t"
		"shll8	%[ag0]\n\t"
		"sts	macl, %[rb1]\n\t"
		"shlr8	%[rb1]\n\t"
		"add	%[rb1], %[rb0]\n\t"
		"and	%[m], %[rb0]\n\t"
		"or		%[ag0], %[rb0]\n"
		"mov.l	%[rb0], @-%[d]\n\t"

		"fmov	dr6,@-%[d]\n\t"
		"fmov	dr4,@-%[d]\n\t"
		"fmov	dr2,@-%[d]\n\t"

		"pref	@%[d]\n\t"
		"fschg\n"
		: [d] "+&r"(dest), [i] "+&r"(inside), [o] "+&r"(outside), [e] "+r"(eos), [rb0] "=&r"(in_rb), [rb1] "=&r"(out_rb), [ag0] "=&r"(in_ag), [ag1] "=&r"(out_ag)
		: [m] "r"(0x00ff00ff)
		: "macl", "fpul", "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7", "fr8", "fr9");

	// unsigned int *flags = (unsigned int *)dest;
	// float iw = 1.0f / inside->z;
	// float ow = 1.0f / outside->z;
	// float inter = (iw - 1.0f) / (iw - ow);
	// float ix = inside->x * iw;
	// float iy = inside->y * iw;
	// float ox = outside->x * ow;
	// float oy = outside->y * ow;
	// unsigned int mask = 0x00ff00ff;
	// unsigned int in_rb = inside->argb & mask;
	// unsigned int in_ag = (inside->argb >> 8) & mask;
	// unsigned int out_rb = outside->argb & mask;
	// unsigned int out_ag = (outside->argb >> 8) & mask;
	// int i_inter = (int)(inter * 255.0f);
	// *flags = (eos + 14) << 28;
	// dest->x = inter * (ox - ix) + ix;
	// dest->y = inter * (oy - iy) + iy;
	// dest->z = 1.0f;
	// dest->u = inter * (outside->u - inside->u) + inside->u;
	// dest->v = inter * (outside->v - inside->v) + inside->v;
	// in_rb += (i_inter * ((int)out_rb - (int)in_rb)) >> 8;
	// in_ag += (i_inter * ((int)out_ag - (int)in_ag)) >> 8;
	// flags[6] = ((in_ag & mask) << 8) | (in_rb & mask);
	// in_rb = inside->oargb & mask;
	// in_ag = (inside->oargb >> 8) & mask;
	// out_rb = outside->oargb & mask;
	// out_ag = (outside->oargb >> 8) & mask;
	// in_rb += (i_inter * ((int)out_rb - (int)in_rb)) >> 8;
	// in_ag += (i_inter * ((int)out_ag - (int)in_ag)) >> 8;
	// flags[7] = ((in_ag & mask) << 8) | (in_rb & mask);
	// asm("pref @%0"
	// 	:
	// 	: "r"(dest));
}

static void inter_vert_commit_intensity(pvr_vertex_t *dest, pvr_vertex_t *inside, pvr_vertex_t *outside, int eos)
{
	asm volatile(
		"fschg\n\t"

		"add	#8, %[i]\n\t"
		"add	#8, %[o]\n\t"
		"fmov.d	@%[i]+, dr2\n\t"
		"fmov.d	@%[i]+, dr4\n\t"
		"fmov.d	@%[i]+, dr6\n\t"
		"fmov.d	@%[o]+, dr8\n\t"
		"fmov.d	@%[o]+, dr10\n\t"
		"fmov.d	@%[o]+, dr12\n\t"

		"fmul	fr3, fr4\n\t"
		"add	#32, %[d]\n\t"
		"fmul	fr3, fr5\n\t"
		"add	#-32, %[i]\n\t"
		"fmul	fr3, fr6\n\t"
		"add	#-32, %[o]\n\t"
		"fmul	fr3, fr7\n\t"
		"fcmp/gt fr9, fr3\n\t"
		"fmul	fr9, fr10\n\t"
		"fldi1	fr0\n\t"
		"fmul	fr9, fr11\n\t"
		"fneg	fr0\n\t"
		"fmul	fr9, fr12\n\t"
		"fmul	fr9, fr13\n\t"

		"fadd	fr3, fr0\n\t"
		"fsub	fr9, fr3\n\t"
		"fmul	fr3, fr3\n\t"
		"bt/s	1f\n\t"
		"fsrra	fr3\n\t"
		"fneg	fr3\n\t"
		"1:\n\t"
		"fmul	fr3, fr0\n\t"

		"fsub	fr4, fr10\n\t"
		"add	#14, %[e]\n\t"
		"fsub	fr5, fr11\n\t"
		"shld	%[shift], %[e]\n\t"
		"fsub	fr6, fr12\n\t"
		"lds	%[e], fpul\n\t"
		"fsub	fr7, fr13\n\t"
		"fmac	fr0, fr10, fr4\n\t"
		"fmac	fr0, fr11, fr5\n\t"
		"fmac	fr0, fr12, fr6\n\t"
		"fmac	fr0, fr13, fr7\n\t"

		"fmov.d	dr6, @-%[d]\n\t"
		"fmov.d	dr4, @-%[d]\n\t"

		"fmov.d	@%[i]+, dr4\n\t"
		"fmov.d @%[o]+, dr6\n\t"
		"fsts	fpul, fr4\n\t"

		"fsub	fr2, fr8\n\t"
		"fsub	fr5, fr7\n\t"
		"fmac	fr0, fr8, fr2\n\t"
		"fmac	fr0, fr7, fr5\n\t"
		"fldi1	fr3\n\t"

		"fmov.d	dr2, @-%[d]\n\t"
		"fmov.d	dr4, @-%[d]\n\t"

		"pref	@%[d]\n\t"
		"fschg\n"
		: [d] "+&r"(dest), [i] "+&r"(inside), [o] "+&r"(outside), [e] "+r"(eos)
		: [shift] "r"(28)
		: "fpul", "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7", "fr8", "fr9", "fr10", "fr11", "fr12", "fr13");

	// unsigned int *flags = (unsigned int *)dest;
	// float *d = (float *)dest;
	// float *inside_color = (float *)&inside->argb;
	// float *outside_color = (float *)&outside->argb;
	// float inter = (inside->z - 1.0f) / (inside->z - outside->z);
	// *flags = (eos + 14) << 28;
	// d[1] = inter * (outside->x - inside->x) + inside->x;
	// d[2] = inter * (outside->y - inside->y) + inside->y;
	// d[3] = 1.0f;
	// d[4] = inter * (outside->u * outside->z - inside->u * inside->z) + inside->u * inside->z;
	// d[5] = inter * (outside->v * outside->z - inside->v * inside->z) + inside->v * inside->z;
	// d[6] = inter * (outside_color[0] * outside->z - inside_color[0] * inside->z) + inside_color[0] * inside->z;
	// d[7] = inter * (outside_color[1] * outside->z - inside_color[1] * inside->z) + inside_color[1] * inside->z;
	// asm("pref @%0"
	// 	:
	// 	: "r"(dest));
}

int pvr_vertex_commit_zclip(pvr_vertex_t *src, int size)
{
	pvr_vertex_t *dest = (pvr_vertex_t *)SQ_MASK_DEST((void *)PVR_TA_INPUT);
	pvr_vertex_t *top = dest;
	sq_lock((void *)PVR_TA_INPUT);
	src = (pvr_vertex_t *)&src->flags;
	for (int strip_num = 2; size; size -= strip_num)
	{
		int clip = 0;
		if (size < 3)
			break;
		/*  First and Second point */
		if (1.0f >= src->z && src->z > 0.0f)
		{
			vert_commit(dest++, src, 0);
			src++;
			if (1.0f >= src->z && src->z > 0.0f)
			{
				/* 0, 1 inside */
				vert_commit(dest++, src, 0);
				clip = 6;
			}
			else
			{
				/* 0 inside, 1 outside */
				inter_vert_commit(dest++, &src[-1], src, 0);
				clip = 2;
			}
		}
		else
		{
			src++;
			if (1.0f >= src->z && src->z > 0.0f)
			{
				/* 0 outside and 1 inside */
				inter_vert_commit(dest++, src, &src[-1], 0);
				vert_commit(dest++, src, 0);
				clip = 4;
			}
		}
		src++;
		/* Third point and more */
		for (int eos = 0; !eos; src++, strip_num++)
		{
			/* End of strip */
			eos = src->flags >> 28 & 1;
			/* Clip code */
			clip >>= 1;
			if (1.0f >= src->z && src->z > 0.0f)
				clip |= 4;
			/* Clipping */
			if (!clip)
			{
				/* all outside */
				continue;
			}
			else if (clip == 7)
			{
				/* all inside */
				vert_commit(dest++, src, eos);
				continue;
			}
			switch (clip)
			{
			case 1: /* 0 inside, 1 and 2 outside */
				/* Pause strip */
				inter_vert_commit(dest++, &src[-2], src, 1);
				break;
			case 3: /* 0 and 1 inside, 2 outside */
				inter_vert_commit(dest++, &src[-2], src, 0);
				vert_commit(dest++, &src[-1], 0);
			case 2: /* 0 outside, 1 inside, 2 outside */
				inter_vert_commit(dest++, &src[-1], src, eos);
				break;
			case 4: /* 0 and 1 outside, 2 inside */
				inter_vert_commit(dest++, src, &src[-2], 0);
				if (strip_num & 0x01)
				{
				case 5: /* 0 inside, 1 outside and 2 inside */
					/* Turn over */
					vert_commit(dest++, src, 0);
				}
				inter_vert_commit(dest++, src, &src[-1], 0);
				vert_commit(dest++, src, eos);
				break;
			case 6: /* 0 outside, 1 and 2 inside */
				inter_vert_commit(dest++, src, &src[-2], 0);
				vert_commit(dest++, &src[-1], 0);
				vert_commit(dest++, src, eos);
				break;
			default:
			}
		}
	}
	sq_unlock();
	return (int)(dest - top) * 8;
}

int pvr_vertex_commit_zclip_intensity(pvr_vertex_t *src, int size)
{
	pvr_vertex_t *dest = (pvr_vertex_t *)SQ_MASK_DEST((void *)PVR_TA_INPUT);
	pvr_vertex_t *top = dest;
	sq_lock((void *)PVR_TA_INPUT);
	src = (pvr_vertex_t *)&src->flags;
	for (int strip_num = 2; size; size -= strip_num)
	{
		int clip = 0;
		if (size < 3)
			break;
		/*  First and Second point */
		if (1.0f >= src->z && src->z > 0.0f)
		{
			vert_commit(dest++, src, 0);
			src++;
			if (1.0f >= src->z && src->z > 0.0f)
			{
				/* 0, 1 inside */
				vert_commit(dest++, src, 0);
				clip = 6;
			}
			else
			{
				/* 0 inside, 1 outside */
				inter_vert_commit_intensity(dest++, &src[-1], src, 0);
				clip = 2;
			}
		}
		else
		{
			src++;
			if (1.0f >= src->z && src->z > 0.0f)
			{
				/* 0 outside and 1 inside */
				inter_vert_commit_intensity(dest++, src, &src[-1], 0);
				vert_commit(dest++, src, 0);
				clip = 4;
			}
		}
		src++;
		/* Third point and more */
		for (int eos = 0; !eos; src++, strip_num++)
		{
			/* End of strip */
			eos = src->flags >> 28 & 1;
			/* Clip code */
			clip >>= 1;
			if (1.0f >= src->z && src->z > 0.0f)
				clip |= 4;
			/* Clipping */
			if (!clip)
			{
				/* all outside */
				continue;
			}
			else if (clip == 7)
			{
				/* all inside */
				vert_commit(dest++, src, eos);
				continue;
			}
			switch (clip)
			{
			case 1: /* 0 inside, 1 and 2 outside */
				/* Pause strip */
				inter_vert_commit_intensity(dest++, &src[-2], src, 1);
				break;
			case 3: /* 0 and 1 inside, 2 outside */
				inter_vert_commit_intensity(dest++, &src[-2], src, 0);
				vert_commit(dest++, &src[-1], 0);
			case 2: /* 0 outside, 1 inside, 2 outside */
				inter_vert_commit_intensity(dest++, &src[-1], src, eos);
				break;
			case 4: /* 0 and 1 outside, 2 inside */
				inter_vert_commit_intensity(dest++, src, &src[-2], 0);
				if (strip_num & 0x01)
				{
				case 5: /* 0 inside, 1 outside and 2 inside */
					/* Turn over */
					vert_commit(dest++, src, 0);
				}
				inter_vert_commit_intensity(dest++, src, &src[-1], 0);
				vert_commit(dest++, src, eos);
				break;
			case 6: /* 0 outside, 1 and 2 inside */
				inter_vert_commit_intensity(dest++, src, &src[-2], 0);
				vert_commit(dest++, &src[-1], 0);
				vert_commit(dest++, src, eos);
				break;
			default:
			}
		}
	}
	sq_unlock();
	return (int)(dest - top) * 8;
}

static void *modi_commit(void *dest, pvr_mod_hdr_t *header, pvr_modifier_vol_t *vol, int eol)
{
	unsigned int *d = dest;
	unsigned int *s = (unsigned int *)&vol->flags;
	static int first = 1;
	if (first)
	{
		if (eol)
			return dest;
		first = 0;
		/* Send first header */
		d[0] = header->cmd & 0xffffffbf;
		d[1] = header->mode1 & 0x9fffffff;
		d[2] = 0;
		d[3] = 0;
		d[4] = 0;
		d[5] = 0;
		d[6] = 0;
		d[7] = 0;
		asm("pref @%0"
			:
			: "r"(d));
		d += 8;
		/* If the first, don't send vertex. */
		return (void *)d;
	}
	if (eol)
	{
		first = 1;
		/* Send header */
		d[0] = header->cmd;
		d[1] = header->mode1;
		d[2] = 0;
		d[3] = 0;
		d[4] = 0;
		d[5] = 0;
		d[6] = 0;
		d[7] = 0;
		asm("pref @%0"
			:
			: "r"(d));
		d += 8;
	}
	/* Send vertex */
	asm volatile(
		"fschg\n\t"
		"fmov.d	@%[s]+, dr4\n\t"
		"add	#32, %[d]\n\t"
		"fmov.d @%[s]+, dr6\n\t"
		"fmov.d @%[s]+, dr8\n\t"
		"fmov.d	@%[s]+, dr10\n\t"
		"fmov.d	dr10, @-%[d]\n\t"
		"fmov.d	dr8, @-%[d]\n\t"
		"fmov.d	dr6, @-%[d]\n\t"
		"fmov.d	dr4, @-%[d]\n\t"
		"pref	@%[d]\n\t"
		"fmov.d	@%[s]+, dr4\n\t"
		"add	#64, %[d]\n\t"
		"fmov.d @%[s]+, dr6\n\t"
		"fmov.d @%[s]+, dr8\n\t"
		"fmov.d	@%[s]+, dr10\n\t"
		"fmov.d	dr10, @-%[d]\n\t"
		"fmov.d	dr8, @-%[d]\n\t"
		"fmov.d	dr6, @-%[d]\n\t"
		"fmov.d	dr4, @-%[d]\n\t"
		"pref	@%[d]\n\t"
		"fschg\n\t"
		"add	#32, %[d]\n"
		: [d] "+&r"(d), [s] "+&r"(s)
		:
		: "memory", "fr4", "fr5", "fr6", "fr7", "fr8", "fr9", "fr10", "fr11");
	// d[0] = s[0];
	// d[1] = s[1];
	// d[2] = s[2];
	// d[3] = s[3];
	// d[4] = s[4];
	// d[5] = s[5];
	// d[6] = s[6];
	// d[7] = s[7];
	// asm("pref @%0"
	// 	:
	// 	: "r"(d));
	// d += 8;
	// d[0] = s[8];
	// d[1] = s[9];
	// d[2] = s[10];
	// d[3] = s[11];
	// d[4] = s[12];
	// d[5] = s[13];
	// d[6] = s[14];
	// d[7] = s[15];
	// asm("pref @%0"
	// 	:
	// 	: "r"(d));
	// d += 8;
	return (void *)d;
}

static inline void inter_vert(float *dest3f, float in_x, float in_y, float in_z, float out_x, float out_y, float out_z)
{
	asm volatile(
		"fcmp/gt %[z0], %[z1]\n\t"
		"add	#12, %[d]\n\t"

		"fldi1	fr0\n\t"
		"fneg	fr0\n\t"
		"fadd	%[z0], fr0\n\t"
		"fsub	%[z1], %[z0]\n\t"
		"fmul	%[z0], %[z0]\n\t"
		"bf/s	1f\n\t"
		"fsrra	%[z0]\n\t"
		"fneg	%[z0]\n\t"
		"1:\n\t"
		"fmul	%[z0], fr0\n\t"

		"fsub	%[x0], %[x1]\n\t"
		"fsub	%[y0], %[y1]\n\t"
		"fldi1	%[z0]\n\t"
		"fmac	fr0, %[x1], %[x0]\n\t"
		"fmac	fr0, %[y1], %[y0]\n\t"

		"fmov.s	%[z0],@-%[d]\n\t"
		"fmov.s	%[y0],@-%[d]\n\t"
		"fmov.s	%[x0],@-%[d]\n\n"
		: [d] "+&r"(dest3f)
		: [x0] "f"(in_x), [y0] "f"(in_y), [z0] "f"(in_z), [x1] "f"(out_x), [y1] "f"(out_y), [z1] "f"(out_z)
		: "memory", "fr0");
	// float inter = (in_z - 1.0f) / (in_z - out_z);
	// dest3f[0] = inter * (out_x - in_x) + in_x;
	// dest3f[1] = inter * (out_y - in_y) + in_y;
	// dest3f[2] = 1.0f;
}

int pvr_modifier_commit_zclip(pvr_mod_hdr_t *eol_header, pvr_modifier_vol_t *vol, int size)
{
	pvr_modifier_vol_t buf = {
		PVR_CMD_VERTEX_EOL,
		0.0f, 0.0f, 0.0f, /* 1st cover vertex */
		0.0f, 0.0f, 0.0f, /* 2nd cover vertex */
		0.0f, 0.0f, 0.0f,
		0, 0, 0, 0, 0, 0};
	float *dest = (float *)SQ_MASK_DEST((void *)PVR_TA_INPUT);
	float *top = dest;
	float cover[3]; /* 3rd cover vertex */
	int cover_flag = 0;
	sq_lock((void *)PVR_TA_INPUT);
	vol = (pvr_modifier_vol_t *)&vol->flags;
	for (int i = size; i; i--, vol++)
	{
		if (1.0f >= vol->az && vol->az > 0.0f)
		{
			if (1.0f >= vol->bz && vol->bz > 0.0f)
			{
				if (1.0f >= vol->cz && vol->cz > 0.0f)
				{
					/* all inside */
					dest = modi_commit(dest, eol_header, &buf, 0);
					buf = *vol;
					continue;
				}
				else
				{
					/* 0 and 1 inside, 2 outside */
					dest = modi_commit(dest, eol_header, &buf, 0);
					inter_vert((float *)&buf.ax, vol->bx, vol->by, vol->bz, vol->cx, vol->cy, vol->cz);
					buf.bx = vol->bx;
					buf.by = vol->by;
					buf.bz = vol->bz;
					buf.cx = vol->ax;
					buf.cy = vol->ay;
					buf.cz = vol->az;
					dest = modi_commit(dest, eol_header, &buf, 0);
					inter_vert((float *)&buf.bx, vol->ax, vol->ay, vol->az, vol->cx, vol->cy, vol->cz);
				}
			}
			else
			{
				if (1.0f >= vol->cz && vol->cz > 0.0f)
				{
					/* 0 inside, 1 outside, 2 inside */
					dest = modi_commit(dest, eol_header, &buf, 0);
					inter_vert((float *)&buf.ax, vol->cx, vol->cy, vol->cz, vol->bx, vol->by, vol->bz);
					buf.bx = vol->cx;
					buf.by = vol->cy;
					buf.bz = vol->cz;
					buf.cx = vol->ax;
					buf.cy = vol->ay;
					buf.cz = vol->az;
					dest = modi_commit(dest, eol_header, &buf, 0);
					inter_vert((float *)&buf.bx, vol->ax, vol->ay, vol->az, vol->bx, vol->by, vol->bz);
				}
				else
				{
					/* 0 inside, 1 and 2 outside */
					dest = modi_commit(dest, eol_header, &buf, 0);
					inter_vert((float *)&buf.ax, vol->ax, vol->ay, vol->az, vol->bx, vol->by, vol->bz);
					inter_vert((float *)&buf.bx, vol->ax, vol->ay, vol->az, vol->cx, vol->cy, vol->cz);
					buf.cx = vol->ax;
					buf.cy = vol->ay;
					buf.cz = vol->az;
				}
			}
		}
		else
		{
			if (1.0f >= vol->bz && vol->bz > 0.0f)
			{
				if (1.0f >= vol->cz && vol->cz > 0.0f)
				{
					/* 0 outside, 1 and 2 inside */
					dest = modi_commit(dest, eol_header, &buf, 0);
					inter_vert((float *)&buf.ax, vol->bx, vol->by, vol->bz, vol->ax, vol->ay, vol->az);
					buf.bx = vol->bx;
					buf.by = vol->by;
					buf.bz = vol->bz;
					buf.cx = vol->cx;
					buf.cy = vol->cy;
					buf.cz = vol->cz;
					dest = modi_commit(dest, eol_header, &buf, 0);
					inter_vert((float *)&buf.bx, vol->cx, vol->cy, vol->cz, vol->ax, vol->ay, vol->az);
				}
				else
				{
					/* 0 outside, 1 inside, 2 outside */
					dest = modi_commit(dest, eol_header, &buf, 0);
					inter_vert((float *)&buf.ax, vol->bx, vol->by, vol->bz, vol->ax, vol->ay, vol->az);
					inter_vert((float *)&buf.bx, vol->bx, vol->by, vol->bz, vol->cx, vol->cy, vol->cz);
					buf.cx = vol->bx;
					buf.cy = vol->by;
					buf.cz = vol->bz;
				}
			}
			else
			{
				if (1.0f >= vol->cz && vol->cz > 0.0f)
				{
					/* 0 and 1 outside, 2 inside */
					dest = modi_commit(dest, eol_header, &buf, 0);
					inter_vert((float *)&buf.ax, vol->cx, vol->cy, vol->cz, vol->ax, vol->ay, vol->az);
					inter_vert((float *)&buf.bx, vol->cx, vol->cy, vol->cz, vol->bx, vol->by, vol->bz);
					buf.cx = vol->cx;
					buf.cy = vol->cy;
					buf.cz = vol->cz;
				}
				else
				{
					/* all outside */
					continue;
				}
			}
		}
		/* Make cover polygon. */
		if (cover_flag)
		{
			dest = modi_commit(dest, eol_header, &buf, 0);
			buf.cx = cover[0];
			buf.cy = cover[1];
			buf.cz = cover[2];
		}
		else
		{
			cover[0] = buf.ax;
			cover[1] = buf.ay;
			cover[2] = buf.az;
			cover_flag = 1;
		}
	}
	/* Send EOL */
	dest = modi_commit(dest, eol_header, &buf, 1);
	sq_unlock();
	return (int)(dest - top) / 4;
}
