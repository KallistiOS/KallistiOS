/* KallistiOS ##version##

   linux/fb.h
   Copyright (C) 2024 Donald Haase
*/

/** \file    linux/fb.h
    \brief   Definitions for a Linux framebuffer device.
    \ingroup dev_fb

    This file contains the definitions for the defines and structs 
    needed to support the ioctl of /dev/fb devices. It is a truncated 
    version of the one provided by linux.
    
    TODO: Before merging ensure that this supports some software that 
    uses the fb.

    \author Donald Haase
*/

#ifndef __LINUX_FB_H
#define __LINUX_FB_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#define FBIOGET_VSCREENINFO	    0x00
#define FBIOPUT_VSCREENINFO	    0x01
#define FBIOGET_FSCREENINFO	    0x02
#define FBIOGETCMAP		        0x04
#define FBIOPUTCMAP		        0x05
#define FBIOPAN_DISPLAY		    0x06
#define FBIO_CURSOR             0x08 //_IOWR('F', 0x08, struct fb_cursor)

#define FBIOGET_CON2FBMAP	    0x0F
#define FBIOPUT_CON2FBMAP	    0x10
#define FBIOBLANK		        0x11		/* arg: 0 or vesa level + 1 */
#define FBIOGET_VBLANK		    0x12 //_IOR('F', 0x12, struct fb_vblank)
#define FBIO_ALLOC              0x13
#define FBIO_FREE               0x14
#define FBIOGET_GLYPH           0x15
#define FBIOGET_HWCINFO         0x16
#define FBIOPUT_MODEINFO        0x17
#define FBIOGET_DISPINFO        0x18
#define FBIO_WAITFORVSYNC	    0x20 // _IOW('F', 0x20, uint32_t)

#define FB_TYPE_PACKED_PIXELS		0	/* Packed Pixels	*/
#define FB_TYPE_PLANES			1	/* Non interleaved planes */
#define FB_TYPE_INTERLEAVED_PLANES	2	/* Interleaved planes	*/
#define FB_TYPE_TEXT			3	/* Text/attributes	*/
#define FB_TYPE_VGA_PLANES		4	/* EGA/VGA planes	*/
#define FB_TYPE_FOURCC			5	/* Type identified by a V4L2 FOURCC */

#define FB_AUX_TEXT_MDA		    0	/* Monochrome text */
#define FB_AUX_TEXT_CGA		    1	/* CGA/EGA/VGA Color text */
#define FB_AUX_TEXT_S3_MMIO	    2	/* S3 MMIO fasttext */
#define FB_AUX_TEXT_MGA_STEP16	3	/* MGA Millenium I: text, attr, 14 reserved bytes */
#define FB_AUX_TEXT_MGA_STEP8	4	/* other MGAs:      text, attr,  6 reserved bytes */
#define FB_AUX_TEXT_SVGA_GROUP	8	/* 8-15: SVGA tileblit compatible modes */
#define FB_AUX_TEXT_SVGA_MASK	7	/* lower three bits says step */
#define FB_AUX_TEXT_SVGA_STEP2	8	/* SVGA text mode:  text, attr */
#define FB_AUX_TEXT_SVGA_STEP4	9	/* SVGA text mode:  text, attr,  2 reserved bytes */
#define FB_AUX_TEXT_SVGA_STEP8	10	/* SVGA text mode:  text, attr,  6 reserved bytes */
#define FB_AUX_TEXT_SVGA_STEP16	11	/* SVGA text mode:  text, attr, 14 reserved bytes */
#define FB_AUX_TEXT_SVGA_LAST	15	/* reserved up to 15 */

#define FB_AUX_VGA_PLANES_VGA4		0	/* 16 color planes (EGA/VGA) */
#define FB_AUX_VGA_PLANES_CFB4		1	/* CFB4 in planes (VGA) */
#define FB_AUX_VGA_PLANES_CFB8		2	/* CFB8 in planes (VGA) */

#define FB_VISUAL_MONO01		        0	/* Monochr. 1=Black 0=White */
#define FB_VISUAL_MONO10		        1	/* Monochr. 1=White 0=Black */
#define FB_VISUAL_TRUECOLOR		        2	/* True color	*/
#define FB_VISUAL_PSEUDOCOLOR		    3	/* Pseudo color (like atari) */
#define FB_VISUAL_DIRECTCOLOR		    4	/* Direct color */
#define FB_VISUAL_STATIC_PSEUDOCOLOR	5	/* Pseudo color readonly */
#define FB_VISUAL_FOURCC		        6	/* Visual identified by a V4L2 FOURCC */

#define FB_ACCEL_NONE		        0	/* no hardware accelerator	*/

struct fb_fix_screeninfo {
	char id[16];			/* identification string eg "TT Builtin" */
	unsigned long smem_start;	/* Start of frame buffer mem */
					/* (physical address) */
	uint32_t smem_len;			/* Length of frame buffer mem */
	uint32_t type;			/* see FB_TYPE_*		*/
	uint32_t type_aux;			/* Interleave for interleaved Planes */
	uint32_t visual;			/* see FB_VISUAL_*		*/ 
	uint16_t xpanstep;			/* zero if no hardware panning  */
	uint16_t ypanstep;			/* zero if no hardware panning  */
	uint16_t ywrapstep;		/* zero if no hardware ywrap    */
	uint32_t line_length;		/* length of a line in bytes    */
	unsigned long mmio_start;	/* Start of Memory Mapped I/O   */
					/* (physical address) */
	uint32_t mmio_len;			/* Length of Memory Mapped I/O  */
	uint32_t accel;			/* Indicate to driver which	*/
					/*  specific chip/card we have	*/
	uint16_t capabilities;		/* see FB_CAP_*			*/
	uint16_t reserved[2];		/* Reserved for future compatibility */
};

/* Interpretation of offset for color fields: All offsets are from the right,
 * inside a "pixel" value, which is exactly 'bits_per_pixel' wide (means: you
 * can use the offset as right argument to <<). A pixel afterwards is a bit
 * stream and is written to video memory as that unmodified.
 *
 * For pseudocolor: offset and length should be the same for all color
 * components. Offset specifies the position of the least significant bit
 * of the palette index in a pixel value. Length indicates the number
 * of available palette entries (i.e. # of entries = 1 << length).
 */
struct fb_bitfield {
	uint32_t offset;			/* beginning of bitfield	*/
	uint32_t length;			/* length of bitfield		*/
	uint32_t msb_right;		/* != 0 : Most significant bit is */ 
					/* right */ 
};

#define FB_NONSTD_HAM		    1	/* Hold-And-Modify (HAM)        */
#define FB_NONSTD_REV_PIX_IN_B	2	/* order of pixels in each byte is reversed */

#define FB_ACTIVATE_NOW		    0	/* set values immediately (or vbl)*/
#define FB_ACTIVATE_NXTOPEN	    1	/* activate on next open	*/
#define FB_ACTIVATE_TEST	    2	/* don't set, round up impossible */
#define FB_ACTIVATE_MASK       15
					/* values			*/
#define FB_ACTIVATE_VBL	       16	/* activate values on next vbl  */
#define FB_CHANGE_CMAP_VBL     32	/* change colormap on vbl	*/
#define FB_ACTIVATE_ALL	       64	/* change all VCs on this fb	*/
#define FB_ACTIVATE_FORCE     128	/* force apply even when no change*/
#define FB_ACTIVATE_INV_MODE  256       /* invalidate videomode */
#define FB_ACTIVATE_KD_TEXT   512       /* for KDSET vt ioctl */

#define FB_ACCELF_TEXT		    1	/* (OBSOLETE) see fb_info.flags and vc_mode */

#define FB_SYNC_HOR_HIGH_ACT	1	/* horizontal sync high active	*/
#define FB_SYNC_VERT_HIGH_ACT	2	/* vertical sync high active	*/
#define FB_SYNC_EXT		        4	/* external sync		*/
#define FB_SYNC_COMP_HIGH_ACT	8	/* composite sync high active   */
#define FB_SYNC_BROADCAST	    16	/* broadcast video timings      */
					/* vtotal = 144d/288n/576i => PAL  */
					/* vtotal = 121d/242n/484i => NTSC */
#define FB_SYNC_ON_GREEN	32	/* sync on green */

#define FB_VMODE_NONINTERLACED  0	/* non interlaced */
#define FB_VMODE_INTERLACED	1	/* interlaced	*/
#define FB_VMODE_DOUBLE		2	/* double scan */
#define FB_VMODE_ODD_FLD_FIRST	4	/* interlaced: top line first */
#define FB_VMODE_MASK		255

#define FB_VMODE_YWRAP		256	/* ywrap instead of panning     */
#define FB_VMODE_SMOOTH_XPAN	512	/* smooth xpan possible (internally used) */
#define FB_VMODE_CONUPDATE	512	/* don't update x/yoffset	*/

/*
 * Display rotation support
 */
#define FB_ROTATE_UR      0
#define FB_ROTATE_CW      1
#define FB_ROTATE_UD      2
#define FB_ROTATE_CCW     3

struct fb_var_screeninfo {
	uint32_t xres;			/* visible resolution		*/
	uint32_t yres;
	uint32_t xres_virtual;		/* virtual resolution		*/
	uint32_t yres_virtual;
	uint32_t xoffset;			/* offset from virtual to visible */
	uint32_t yoffset;			/* resolution			*/

	uint32_t bits_per_pixel;		/* guess what			*/
	uint32_t grayscale;		/* 0 = color, 1 = grayscale,	*/
					/* >1 = FOURCC			*/
	struct fb_bitfield red;		/* bitfield in fb mem if true color, */
	struct fb_bitfield green;	/* else only length is significant */
	struct fb_bitfield blue;
	struct fb_bitfield transp;	/* transparency			*/	

	uint32_t nonstd;			/* != 0 Non standard pixel format */

	uint32_t activate;			/* see FB_ACTIVATE_*		*/

	uint32_t height;			/* height of picture in mm    */
	uint32_t width;			/* width of picture in mm     */

	uint32_t accel_flags;		/* (OBSOLETE) see fb_info.flags */

	/* Timing: All values in pixclocks, except pixclock (of course) */
	uint32_t pixclock;			/* pixel clock in ps (pico seconds) */
	uint32_t left_margin;		/* time from sync to picture	*/
	uint32_t right_margin;		/* time from picture to sync	*/
	uint32_t upper_margin;		/* time from sync to picture	*/
	uint32_t lower_margin;
	uint32_t hsync_len;		/* length of horizontal sync	*/
	uint32_t vsync_len;		/* length of vertical sync	*/
	uint32_t sync;			/* see FB_SYNC_*		*/
	uint32_t vmode;			/* see FB_VMODE_*		*/
	uint32_t rotate;			/* angle we rotate counter clockwise */
	uint32_t colorspace;		/* colorspace for FOURCC-based modes */
	uint32_t reserved[4];		/* Reserved for future compatibility */
};

struct fb_cmap {
	uint32_t start;			/* First entry	*/
	uint32_t len;			/* Number of entries */
	uint16_t *red;			/* Red values	*/
	uint16_t *green;
	uint16_t *blue;
	uint16_t *transp;			/* transparency, can be NULL */
};

__END_DECLS

#endif /* !__LINUX_FB_H */